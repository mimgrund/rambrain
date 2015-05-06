#include "managedMemory.h"
#include "common.h"
#include "exceptions.h"
#include "dummyManagedMemory.h"
#include <sys/signal.h>

namespace membrain
{

dummyManagedMemory dummy;

managedMemory *managedMemory::dummyManager = &dummy;
managedMemory *managedMemory::defaultManager = managedMemory::dummyManager;
memoryID const managedMemory::root = 1;
memoryID const managedMemory::invalid = 0;
memoryID managedMemory::parent = 1;
bool managedMemory::noThrow = false;

managedMemory::managedMemory ( managedSwap *swap, unsigned int size  )
{
    memory_max = size;
    defaultManager = this;
    managedMemoryChunk *chunk = mmalloc ( 0 );                //Create root element.
    chunk->status = MEM_ROOT;
    this->swap = swap;
    if ( !swap ) {
        throw incompleteSetupException ( "no swap manager defined" );
    }
#ifdef SWAPSTATS
    instance = this;
    signal ( SIGUSR1, sigswapstats );
#endif
}

managedMemory::~managedMemory()
{
    bool oldthrow = noThrow;
    noThrow = true;
    if ( defaultManager == this ) {
        defaultManager = dummyManager;
    }
    //Clean up objects:
    recursiveMfree ( root );
    noThrow = oldthrow;
}


unsigned int managedMemory::getMemoryLimit () const
{
    return memory_max;
}
unsigned int managedMemory::getUsedMemory() const
{
    return memory_used;
}
unsigned int managedMemory::getSwappedMemory() const
{
    return memory_swapped;
}



bool managedMemory::setMemoryLimit ( unsigned int size )
{
    if ( size > memory_max ) {
        memory_max = size;
        return true;
    } else {
        if ( size < memory_used ) {
            //Try to swap out as much memory as needed:
            unsigned int tobefreed = ( memory_used - size );
            if ( !swapOut ( tobefreed ) ) {
                return false;
            }
            memory_max = size;
            swapOut ( 0 ); // Swap out further to adapt to new memory limits. This must not necessarily succeed.
            return true;
        } else {
            memory_max = size;
            return true;
        }
    }
    return false;                                             //Resetting this is not implemented yet.
}

void managedMemory::ensureEnoughSpaceFor ( unsigned int sizereq )
{
    if ( sizereq + memory_used > memory_max ) {
        if ( !swapOut ( sizereq + memory_used - memory_max ) ) {
            Throw ( memoryException ( "Could not swap memory" ) );
        }
    }
}


managedMemoryChunk *managedMemory::mmalloc ( unsigned int sizereq )
{
    ensureEnoughSpaceFor ( sizereq );

    //We are left with enough free space to malloc.
    managedMemoryChunk *chunk = new managedMemoryChunk ( parent, memID_pace++ );
    chunk->status = MEM_ALLOCATED;
    if ( sizereq != 0 ) {
        chunk->locPtr = malloc ( sizereq );
        if ( !chunk->locPtr ) {
            Throw ( memoryException ( "Malloc failed" ) );
            return NULL;
        }
    }


    memory_used += sizereq;
    chunk->size = sizereq;
    chunk->child = invalid;
    chunk->parent = parent;
    chunk->swapBuf = NULL;
    if ( chunk->id == root ) {                                //We're inserting root elem.
        chunk->next = invalid;
        chunk->atime = 0;
    } else {
        //Register this chunk in swapping logic:
        chunk->atime = atime++;
        schedulerRegister ( *chunk );
        touch ( *chunk );

        //fill in tree:
        managedMemoryChunk &pchunk = resolveMemChunk ( parent );
        if ( pchunk.child == invalid ) {
            chunk->next = invalid;                                //Einzelkind
            pchunk.child = chunk->id;
        } else {
            chunk->next = pchunk.child;
            pchunk.child = chunk->id;
        }

    }

    memChunks.insert ( {chunk->id, chunk} );
    return chunk;
}

bool managedMemory::swapIn ( memoryID id )
{
    managedMemoryChunk chunk = resolveMemChunk ( id );
    return swapIn ( chunk );
}

bool managedMemory::setUse ( managedMemoryChunk &chunk, bool writeAccess = false )
{
    switch ( chunk.status ) {
    case MEM_ALLOCATED_INUSE_READ:
        if ( writeAccess ) {
            chunk.status = MEM_ALLOCATED_INUSE_WRITE;
        }
    case MEM_ALLOCATED_INUSE_WRITE:

#ifdef SWAPSTATS
        ++swap_hits;
#endif
        ++chunk.useCnt;


        touch ( chunk );
        return true;

    case MEM_ALLOCATED:
#ifdef SWAPSTATS
        ++swap_hits;
#endif
        chunk.status = writeAccess ? MEM_ALLOCATED_INUSE_WRITE : MEM_ALLOCATED_INUSE_READ;
        chunk.useCnt = 1;
        touch ( chunk );
        return true;

    case MEM_SWAPPED:
#ifdef SWAPSTATS
        ++swap_misses;
        --swap_hits;
#endif
        if ( swapIn ( chunk ) ) {

            return setUse ( chunk , writeAccess );
        } else {
            return Throw ( memoryException ( "Could not swap memory" ) );
        }

    case MEM_ROOT:
        return false;

    }
    return false;
}

bool managedMemory::mrealloc ( memoryID id, unsigned int sizereq )
{
    managedMemoryChunk &chunk = resolveMemChunk ( id );
    if ( !setUse ( chunk ) ) {
        return false;
    }
    void *realloced = realloc ( chunk.locPtr, sizereq );
    if ( realloced ) {
        memory_used -= chunk.size - sizereq;
        chunk.size = sizereq;
        chunk.locPtr = realloced;
        unsetUse ( chunk );
        return true;
    } else {
        unsetUse ( chunk );
        return false;
    }
}


bool managedMemory::unsetUse ( memoryID id )
{
    managedMemoryChunk chunk = resolveMemChunk ( id );
    return unsetUse ( chunk );
}


bool managedMemory::setUse ( memoryID id )
{
    managedMemoryChunk chunk = resolveMemChunk ( id );
    return setUse ( chunk );
}

bool managedMemory::unsetUse ( managedMemoryChunk &chunk , unsigned int no_unsets )
{
    if ( chunk.status & MEM_ALLOCATED_INUSE_READ ) {
        chunk.useCnt -= no_unsets;
        chunk.status = ( chunk.useCnt == 0 ? MEM_ALLOCATED : chunk.status );
        return true;
    } else {
        return Throw ( memoryException ( "Can not unset use of not used memory" ) );
    }
}

bool managedMemory::Throw ( memoryException e )
{
    if ( noThrow ) {
        errmsg ( e.what() );
        return false;
    } else {
        throw e;
    }
}



void managedMemory::mfree ( memoryID id )
{
    managedMemoryChunk *chunk = memChunks[id];
    if ( chunk->status & MEM_ALLOCATED_INUSE_READ ) {
        Throw ( memoryException ( "Can not free memory which is in use" ) );

        return;
    }
    if ( chunk->child != invalid ) {
        Throw ( memoryException ( "Can not free memory which has active children" ) );
        return;
    }

    if ( chunk ) {
        if ( chunk->id != root ) {
            schedulerDelete ( *chunk );
        }

        if ( chunk->status == MEM_ALLOCATED ) {
            free ( chunk->locPtr );
            memory_used -= chunk->size;
        } else {
            swap->swapDelete ( chunk );
        }

        //get rid of hierarchy:
        if ( chunk->id != root ) {
            managedMemoryChunk *pchunk = &resolveMemChunk ( chunk->parent );
            if ( pchunk->child == chunk->id ) {
                pchunk->child = chunk->next;
            } else {
                pchunk = &resolveMemChunk ( pchunk->child );
                do {
                    if ( pchunk->next == chunk->id ) {
                        pchunk->next = chunk->next;
                        break;
                    };
                    if ( pchunk->next == invalid ) {
                        break;
                    } else {
                        pchunk = &resolveMemChunk ( pchunk->next );
                    }
                } while ( 1 == 1 );
            }
        }

        //Delete element itself
        memChunks.erase ( id );
        delete ( chunk );
    }
}

void managedMemory::recursiveMfree ( memoryID id )
{
    managedMemoryChunk *oldchunk = &resolveMemChunk ( id );
    managedMemoryChunk *next;
    do {
        if ( oldchunk->child != invalid ) {
            recursiveMfree ( oldchunk->child );
        }
        if ( oldchunk->next != invalid ) {
            next = &resolveMemChunk ( oldchunk->next );
        } else {
            break;
        }
        mfree ( oldchunk->id );
        oldchunk = next;
    } while ( 1 == 1 );
    mfree ( oldchunk->id );
}




unsigned int managedMemory::getNumberOfChildren ( const memoryID &id )
{
    const managedMemoryChunk &chunk = resolveMemChunk ( id );
    if ( chunk.child == invalid ) {
        return 0;
    }
    unsigned int no = 1;
    const managedMemoryChunk *child = &resolveMemChunk ( chunk.child );
    while ( child->next != invalid ) {
        child = &resolveMemChunk ( child->next );
        no++;
    }
    return no;
}

void managedMemory::printTree ( managedMemoryChunk *current, unsigned int nspaces )
{
    if ( !current ) {
        current = &resolveMemChunk ( root );
    }
    do {
        for ( unsigned int n = 0; n < nspaces; n++ ) {
            printf ( "  " );
        }
        printf ( "(%d : size %d Bytes, atime %d, ", current->id, current->size, current->atime );
        switch ( current->status ) {
        case MEM_ROOT:
            printf ( "Root Element" );
            break;
        case MEM_SWAPPED:
            printf ( "Swapped out" );
            break;
        case MEM_ALLOCATED:
            printf ( "Allocated" );
            break;
        case MEM_ALLOCATED_INUSE_WRITE:
            printf ( "Allocated&inUse (writable)" );
            break;
        case MEM_ALLOCATED_INUSE_READ:
            printf ( "Allocated&inUse (readonly)" );
            break;
        }
        printf ( ")\n" );
        if ( current->child != invalid ) {
            printTree ( &resolveMemChunk ( current->child ), nspaces + 1 );
        }
        if ( current->next != invalid ) {
            current = &resolveMemChunk ( current->next );
        } else {
            break;
        };
    } while ( 1 == 1 );
}

managedMemoryChunk &managedMemory::resolveMemChunk ( const memoryID &id )
{
    return *memChunks[id];
}

#ifdef SWAPSTATS
void managedMemory::printSwapstats()
{
    infomsgf ( "A total of %d swapouts occured, writing out %d bytes (%.3e Bytes/avg)\
          \n\tA total of %d swapins occured, reading in %d bytes (%.3e Bytes/avg)\
          \n\twe used already loaded elements %d times, %d had to be fetched\
          \n\tthus, the hits over misses rate was %.5f\
          \n\tfraction of swapped out ram (currently) %.2e", n_swap_out, swap_out_bytes, \
               ( ( float ) swap_out_bytes ) / n_swap_out, n_swap_in, swap_in_bytes, ( ( float ) swap_in_bytes ) / n_swap_in, \
               swap_hits, swap_misses, ( ( float ) swap_hits / swap_misses ), ( ( float ) memory_swapped ) / ( memory_used + memory_swapped ) );
}

void managedMemory::resetSwapstats()
{
    swap_hits = swap_misses = swap_in_bytes = swap_out_bytes = n_swap_in = n_swap_out = 0;
}

managedMemory *managedMemory::instance = NULL;
void managedMemory::sigswapstats ( int signum )
{
    printf ( "%ld\t%ld\t%ld\t%ld\t%e\n", instance->swap_out_bytes,
             instance->swap_out_bytes - instance->swap_out_bytes_last,
             instance->swap_in_bytes,
             instance->swap_in_bytes - instance->swap_in_bytes_last, ( double ) instance->swap_hits / instance->swap_misses );
    instance->swap_out_bytes_last = instance->swap_out_bytes;
    instance->swap_in_bytes_last = instance->swap_in_bytes;
}

}

#endif

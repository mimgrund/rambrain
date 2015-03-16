#include "managedMemory.h"
#include "common.h"
#include "exceptions.h"
#include "dummyManagedMemory.h"

managedMemory* managedMemory::dummyManager = new dummyManagedMemory();
managedMemory* managedMemory::defaultManager = managedMemory::dummyManager;
memoryID const managedMemory::root=1;
memoryID const managedMemory::invalid=0;
memoryID managedMemory::parent=1;

managedMemory::managedMemory (managedSwap *swap, unsigned int size  )
{
    memory_max = size;
    if (defaultManager != dummyManager) {
        delete defaultManager;
    }
    defaultManager = this;
    managedMemoryChunk *chunk = mmalloc ( 0 );                //Create root element.
    chunk->status = MEM_ROOT;
    this->swap = swap;
    if(!swap) {
        errmsg("You need to define a swap manager!");
        throw incompleteSetupException("no swap manager defined");
    }
}

managedMemory::~managedMemory()
{
    if ( defaultManager==this) {
        defaultManager = dummyManager;
    }
    //Clean up objects:
    recursiveMfree ( root );
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
    return false;                                             //Resetting this is not implemented yet.
}

void managedMemory::ensureEnoughSpaceFor ( unsigned int sizereq )
{
    if ( sizereq+memory_used>memory_max ) {
        if ( !swapOut ( sizereq ) ) {
            errmsgf ( "Could not swap out >%d Bytes\nOut of Memory.",sizereq );
            throw memoryException("Could not swap memory");
        }
    }
}


managedMemoryChunk* managedMemory::mmalloc ( unsigned int sizereq )
{
    ensureEnoughSpaceFor(sizereq);

    //We are left with enough free space to malloc.
    managedMemoryChunk *chunk = new managedMemoryChunk ( parent,memID_pace++ );
    chunk->status = MEM_ALLOCATED;
    if(sizereq!=0) {
        chunk->locPtr = malloc ( sizereq );
        if ( !chunk->locPtr ) {
            errmsgf ( "Classical malloc on a size of %d failed.",sizereq );
            throw memoryException("Malloc failed");
        }
    }


    memory_used += sizereq;
    chunk->size = sizereq;
    chunk->child = invalid;
    chunk->parent = parent;
    if ( chunk->id==root ) {                                  //We're inserting root elem.
        chunk->next =invalid;
        chunk->atime = 0;
    } else {
        //Register this chunk in swapping logic:
        chunk->atime =atime++;
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

    memChunks.insert ( {chunk->id,chunk} );
    return chunk;
}

bool managedMemory::swapIn ( memoryID id )
{
    managedMemoryChunk chunk = resolveMemChunk ( id );
    return swapIn ( chunk );
}

bool managedMemory::setUse ( managedMemoryChunk& chunk )
{
    switch ( chunk.status ) {
    case MEM_ALLOCATED_INUSE:
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
        chunk.status = MEM_ALLOCATED_INUSE;
        chunk.useCnt = 1;
        touch ( chunk );
        return true;

    case MEM_SWAPPED:
#ifdef SWAPSTATS
        ++swap_misses;
        --swap_hits;
#endif
        if ( swapIn ( chunk ) ) {

            return setUse ( chunk );
        } else {
            errmsgf("Could not swap in a chunk of size %d",chunk.size);
            throw memoryException("Could not swap memory");
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
        memory_used -= chunk.size -sizereq;
        chunk.size = sizereq;
        chunk.locPtr =realloced;
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

bool managedMemory::unsetUse ( managedMemoryChunk& chunk )
{
    if ( chunk.status==MEM_ALLOCATED_INUSE ) {

        chunk.status = ( --chunk.useCnt == 0 ? MEM_ALLOCATED : MEM_ALLOCATED_INUSE );
        return true;
    } else {
        throw unexpectedStateException("Can not unset use of not used memory");
    }
}


void managedMemory::mfree ( memoryID id )
{
    managedMemoryChunk * chunk = memChunks[id];
    if ( chunk->status==MEM_ALLOCATED_INUSE ) {
        errmsg ( "Trying to free memory that is in use." );
        return;
    }
    if ( chunk->child!=invalid ) {
        errmsg ( "Trying to free memory that has still alive children." );
        return;
    }

    if ( chunk ) {
        if(chunk->id!=root)
            schedulerDelete ( *chunk );
        if ( chunk->status==MEM_ALLOCATED ) {
            free ( chunk->locPtr );
            memory_used-= chunk->size;
        }

        //get rid of hierarchy:
        if ( chunk->id!=root ) {
            managedMemoryChunk* pchunk = &resolveMemChunk ( chunk->parent );
            if ( pchunk->child == chunk->id ) {
                pchunk->child = chunk->next;
            } else {
                pchunk = &resolveMemChunk ( pchunk->child );
                do {
                    if ( pchunk->next == chunk->id ) {
                        pchunk->next = chunk->next;
                        break;
                    };
                    if(pchunk->next==invalid)
                        break;
                    else
                        pchunk = &resolveMemChunk(pchunk->next);
                } while ( 1==1 );
            }
        }

        //Delete element itself
        memChunks.erase ( id );
        delete chunk;
    }
}

void managedMemory::recursiveMfree ( memoryID id )
{
    managedMemoryChunk *oldchunk = &resolveMemChunk ( id );
    managedMemoryChunk *next;
    do {
        if ( oldchunk->child!=invalid ) {
            recursiveMfree ( oldchunk->child );
        }
        if ( oldchunk->next!=invalid ) {
            next= &resolveMemChunk ( oldchunk->next );
        } else {
            break;
        }
        mfree ( oldchunk->id );
        oldchunk = next;
    } while ( 1==1 );
    mfree ( oldchunk->id );
}




unsigned int managedMemory::getNumberOfChildren ( const memoryID& id )
{
    const managedMemoryChunk &chunk = resolveMemChunk ( id );
    if ( chunk.child==invalid ) {
        return 0;
    }
    unsigned int no=1;
    const managedMemoryChunk *child = &resolveMemChunk ( chunk.child );
    while ( child->next!=invalid ) {
        child = &resolveMemChunk ( child->next );
        no++;
    }
    return no;
}

void managedMemory::printTree ( managedMemoryChunk *current,unsigned int nspaces )
{
    if ( !current ) {
        current = &resolveMemChunk ( root );
    }
    do {
        for ( unsigned int n=0; n<nspaces; n++ ) {
            printf ( "  " );
        }
        printf ( "(%d : size %d Bytes, atime %d, ",current->id,current->size,current->atime );
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
        case MEM_ALLOCATED_INUSE:
            printf ( "Allocated&inUse" );
            break;
        }
        printf ( ")\n" );
        if ( current->child!=invalid ) {
            printTree ( &resolveMemChunk ( current->child ),nspaces+1 );
        }
        if ( current->next!=invalid ) {
            current = &resolveMemChunk ( current->next );
        } else {
            break;
        };
    } while ( 1==1 );
}


//memoryChunk class:

managedMemoryChunk::managedMemoryChunk ( const memoryID& parent, const memoryID& me )
    : useCnt ( 0 ), parent ( parent ),id ( me )
{
}

managedMemoryChunk& managedMemory::resolveMemChunk ( const memoryID& id )
{
    return *memChunks[id];
}

#ifdef SWAPSTATS
void managedMemory::printSwapstats()
{
    infomsgf("A total of %d swapouts occured, writing out %d bytes (%.3e Bytes/avg)\
          \n\tA total of %d swapins occured, reading in %d bytes (%.3e Bytes/avg)\
          \n\twe used already loaded elements %d times, %d had to be fetched\
          \n\tthus, the hits over misses rate was %.5f\
          \n\tfraction of swapped out ram (currently) %.2e",n_swap_out,swap_out_bytes,\
             ((float)swap_out_bytes)/n_swap_out,n_swap_in,swap_in_bytes,((float)swap_in_bytes)/n_swap_in,\
             swap_hits,swap_misses, ((float)swap_hits/swap_misses),((float)memory_swapped)/(memory_used+memory_swapped));
}

void managedMemory::resetSwapstats()
{
    swap_hits=swap_misses=swap_in_bytes=swap_out_bytes=n_swap_in=n_swap_out=0;
}

#endif

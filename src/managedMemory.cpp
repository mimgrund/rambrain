#include "managedMemory.h"
#include "common.h"
#include "exceptions.h"
#include "dummyManagedMemory.h"
#include <sys/signal.h>
#include "membrainconfig.h"
#include "membrain_atomics.h"
#include "git_info.h"

namespace membrain
{
namespace membrainglobals
{
membrainConfig config;
}

managedMemory *managedMemory::defaultManager;

#ifdef PARENTAL_CONTROL
memoryID const managedMemory::root = 1;
memoryID managedMemory::parent = managedMemory::root;
bool managedMemory::threadSynced = false;
pthread_mutex_t managedMemory::parentalMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t managedMemory::parentalCond = PTHREAD_COND_INITIALIZER;
pthread_t managedMemory::creatingThread ;
bool managedMemory::haveCreatingThread = false;
#endif
memoryID const managedMemory::invalid = 0;




pthread_mutex_t managedMemory::stateChangeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t managedMemory::swappingCond = PTHREAD_COND_INITIALIZER;
unsigned int managedMemory::arrivedSwapins = 0;

managedMemory::managedMemory ( managedSwap *swap, global_bytesize size  )
{
    memory_max = size;
    previousManager = defaultManager;
    defaultManager = this;
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = mmalloc ( 0 );                //Create root element.
    chunk->status = MEM_ROOT;
    haveCreatingThread = false;

#endif
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

    if ( defaultManager == this ) {

        defaultManager = previousManager;
    }
#ifdef PARENTAL_CONTROL
    //Clean up objects:
    recursiveMfree ( root );
#else
    linearMfree();
#endif

}


global_bytesize managedMemory::getMemoryLimit () const
{
    return memory_max;
}
global_bytesize managedMemory::getUsedMemory() const
{
    return memory_used;
}
global_bytesize managedMemory::getSwappedMemory() const
{
    return memory_swapped;
}



bool managedMemory::setMemoryLimit ( global_bytesize size )
{
    if ( size > memory_max ) {
        memory_max = size;
        return true;
    } else {
        if ( size < memory_used ) {
            //Try to swap out as much memory as needed:
            global_bytesize tobefreed = ( memory_used - size );
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

void managedMemory::ensureEnoughSpaceAndLockTopo ( membrain::global_bytesize sizereq )
{
    if ( sizereq + memory_used > memory_max ) {
        if ( !swapOut ( sizereq + memory_used - memory_max ) ) { //Execute swapOut in protected context
            pthread_mutex_unlock ( &stateChangeMutex );
            Throw ( memoryException ( "Could not swap memory" ) );
        } else {
            //We'll wait for possible swapOut to actually occur:
            while ( sizereq + memory_used > memory_max ) {
                pthread_cond_wait ( &swappingCond, &stateChangeMutex );
            }
        }
    }
}


managedMemoryChunk *managedMemory::mmalloc ( global_bytesize sizereq )
{
    pthread_mutex_lock ( &stateChangeMutex );
    ensureEnoughSpaceAndLockTopo ( sizereq );

    memory_used += sizereq;

    //We are left with enough free space to malloc.
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = new managedMemoryChunk ( parent, memID_pace++ );
#else
    managedMemoryChunk *chunk = new managedMemoryChunk ( memID_pace++ );
#endif
    chunk->status = MEM_ALLOCATED;
    chunk->size = sizereq;
#ifdef PARENTAL_CONTROL
    chunk->child = invalid;
    chunk->parent = parent;
    chunk->swapBuf = NULL;
#endif
#ifdef PARENTAL_CONTROL
    if ( chunk->id == root ) {                                //We're inserting root elem.

        chunk->next = invalid;

        chunk->atime = 0;
    } else {
#endif
        //Register this chunk in swapping logic:
        chunk->atime = atime++;
        schedulerRegister ( *chunk );
        touch ( *chunk );
#ifdef PARENTAL_CONTROL
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
#endif

    memChunks.insert ( {chunk->id, chunk} );
    if ( sizereq != 0 ) {
        chunk->locPtr = malloc ( sizereq );
        if ( !chunk->locPtr ) {
            Throw ( memoryException ( "Malloc failed" ) );
            pthread_mutex_unlock ( &stateChangeMutex );
            return NULL;
        }
    }
    pthread_mutex_unlock ( &stateChangeMutex );
    return chunk;
}

bool managedMemory::swapIn ( memoryID id )
{
    managedMemoryChunk chunk = resolveMemChunk ( id );
    return swapIn ( chunk );
}

bool managedMemory::setUse ( managedMemoryChunk &chunk, bool writeAccess = false )
{
    pthread_mutex_lock ( &stateChangeMutex );
    switch ( chunk.status ) {
    case MEM_SWAPOUT: // Object is about to be swapped out.
        waitForSwapout ( chunk, true );
    case MEM_SWAPPED:
        if ( !swapIn ( chunk ) ) {
            return false;
        }
#ifdef SWAPSTATS
        ++swap_misses;
        --swap_hits;
#endif
    case MEM_SWAPIN: // Wait for object to appear
        waitForSwapin ( chunk, true );
    case MEM_ALLOCATED:
        chunk.status = MEM_ALLOCATED_INUSE_READ;
    case MEM_ALLOCATED_INUSE:
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
        pthread_mutex_unlock ( &stateChangeMutex );
        return true;
    case MEM_ROOT:
        pthread_mutex_unlock ( &stateChangeMutex );
        return false;

    }
    return false;
}

bool managedMemory::mrealloc ( memoryID id, global_bytesize sizereq )
{
    managedMemoryChunk &chunk = resolveMemChunk ( id );
    if ( !setUse ( chunk ) ) {
        return false;
    }
    pthread_mutex_lock ( &stateChangeMutex );
    void *realloced = realloc ( chunk.locPtr, sizereq );
    if ( realloced ) {
        memory_used -= chunk.size - sizereq;
        chunk.size = sizereq;
        chunk.locPtr = realloced;
        pthread_mutex_unlock ( &stateChangeMutex );
        unsetUse ( chunk );
        return true;
    } else {
        pthread_mutex_unlock ( &stateChangeMutex );
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
    pthread_mutex_lock ( &stateChangeMutex );
    if ( chunk.status & MEM_ALLOCATED_INUSE_READ ) {
        chunk.useCnt -= no_unsets;
        chunk.status = ( chunk.useCnt == 0 ? MEM_ALLOCATED : chunk.status );
        pthread_mutex_unlock ( &stateChangeMutex );
        return true;
    } else {
        pthread_mutex_unlock ( &stateChangeMutex );
        return Throw ( memoryException ( "Can not unset use of not used memory" ) );
    }
}

bool managedMemory::Throw ( memoryException e )
{

#ifdef PARENTAL_CONTROL
    if ( haveCreatingThread && pthread_equal ( pthread_self(), creatingThread ) ) {
        pthread_mutex_unlock ( &parentalMutex );
    }
#endif
    throw e;
}



void managedMemory::mfree ( memoryID id )
{
    pthread_mutex_lock ( &stateChangeMutex );
    managedMemoryChunk *chunk = memChunks[id];
    if ( chunk->status & MEM_ALLOCATED_INUSE ) {
        pthread_mutex_unlock ( &stateChangeMutex );
        Throw ( memoryException ( "Can not free memory which is in use" ) );
        return;
    }



#ifdef PARENTAL_CONTROL
    if ( chunk->child != invalid ) {
        pthread_mutex_unlock ( &stateChangeMutex );
        Throw ( memoryException ( "Can not free memory which has active children" ) );
        return;
    }
    if ( chunk->id != root ) {
        schedulerDelete ( *chunk );
        if ( chunk->status == MEM_ALLOCATED ) {
            free ( chunk->locPtr );
            memory_used -= chunk->size;
        } else {
            swap->swapDelete ( chunk );
        }
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
#else
    schedulerDelete ( *chunk );
    if ( chunk->status == MEM_ALLOCATED ) {
        free ( chunk->locPtr );
        memory_used -= chunk->size;
    } else {
        swap->swapDelete ( chunk );
    }
#endif
    //Delete element itself
    memChunks.erase ( id );
    delete ( chunk );
    pthread_mutex_unlock ( &stateChangeMutex );
}

#ifdef PARENTAL_CONTROL

unsigned int managedMemory::getNumberOfChildren ( const memoryID &id )
{
    pthread_mutex_lock ( &stateChangeMutex );
    const managedMemoryChunk &chunk = resolveMemChunk ( id );
    if ( chunk.child == invalid ) {
        pthread_mutex_unlock ( &stateChangeMutex );
        return 0;
    }
    unsigned int no = 1;
    const managedMemoryChunk *child = &resolveMemChunk ( chunk.child );
    while ( child->next != invalid ) {
        child = &resolveMemChunk ( child->next );
        no++;
    }
    pthread_mutex_unlock ( &stateChangeMutex );
    return no;
}


void managedMemory::printTree ( managedMemoryChunk *current, unsigned int nspaces )
{
    pthread_mutex_lock ( &stateChangeMutex );
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
    pthread_mutex_unlock ( &stateChangeMutex );
}

///\note This function does not preserve correct deallocation order in class hierarchies, as such, it is not a valid garbage collector.
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
#else
///\note This function does not preserve correct deallocation order in class hierarchies, as such, it is not a valid garbage collector.

void managedMemory::linearMfree()
{
    if ( memChunks.size() == 0 ) {
        return;
    }
    auto it = memChunks.begin();
    do {
        mfree ( it->first );
    } while ( ++it != memChunks.end() );
}

#endif
managedMemoryChunk &managedMemory::resolveMemChunk ( const memoryID &id )
{
    return *memChunks[id];
}

#ifdef SWAPSTATS
void managedMemory::printSwapstats()
{
    infomsgf ( "A total of %lu swapouts occured, writing out %lu bytes (%.3e Bytes/avg)\
          \n\tA total of %lu swapins occured, reading in %lu bytes (%.3e Bytes/avg)\
          \n\twe used already loaded elements %lu times, %lu had to be fetched\
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
#endif

void managedMemory::versionInfo()
{

#ifdef SWAPSTATS
    const char *swapstats = "with swapstats";
#else
    const char *swapstats = "without swapstats";
#endif
#ifdef PARENTAL_CONTROL
    const char *parentalcontrol = "with parental control";
#else
    const char *parentalcontrol = "without parental_control";
#endif

    infomsgf ( "compiled from %s\n\ton %s at %s\n\
                \t%s , %s\n\
    \n \t git diff\n%s\n", gitCommit, __DATE__, __TIME__, swapstats, parentalcontrol, gitDiff );


}

void managedMemory::signalSwappingCond()
{
    arrivedSwapins += 1;
    pthread_cond_broadcast ( &swappingCond );
}

bool managedMemory::waitForSwapin ( managedMemoryChunk &chunk, bool keepSwapLock )
{
    if ( chunk.status & MEM_ALLOCATED ) { //We would wait indefinitely, as the chunk is not about to appear
        if ( !keepSwapLock ) {
            pthread_mutex_unlock ( &stateChangeMutex );
        }
        return true;
    }
    while ( ! ( chunk.status & MEM_ALLOCATED ) ) {
        pthread_cond_wait ( &swappingCond, &stateChangeMutex );
    }
    if ( !keepSwapLock ) {
        pthread_mutex_unlock ( &stateChangeMutex );
    }
    return true;
}


bool managedMemory::waitForSwapout ( managedMemoryChunk &chunk, bool keepSwapLock )
{
    if ( chunk.status  & MEM_SWAPPED ) { //We would wait indefinitely, as the chunk is not about to appear
        if ( !keepSwapLock ) {
            pthread_mutex_unlock ( &stateChangeMutex );
        }
        return false;
    }
    while ( ! ( chunk.status & MEM_SWAPPED ) ) {
        pthread_cond_wait ( &swappingCond, &stateChangeMutex );
    }
    if ( !keepSwapLock ) {
        pthread_mutex_unlock ( &stateChangeMutex );
    }
    return true;
}


}




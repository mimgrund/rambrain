/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "managedMemory.h"
#include "common.h"
#include "exceptions.h"
#include "dummyManagedMemory.h"
#include <sys/signal.h>
#include "rambrainconfig.h"
#include "rambrain_atomics.h"
#include "git_info.h"
#include <time.h>
#include <mm_malloc.h>

namespace rambrain
{
namespace rambrainglobals
{
rambrainConfig config;
}

managedMemory *managedMemory::defaultManager;

#ifdef SWAPSTATS
#ifdef LOGSTATS
FILE *managedMemory::logFile = fopen ( "rambrain-swapstats.log", "w" );
bool managedMemory::firstLog = true;
#endif
#endif

memoryID const managedMemory::invalid = 0;
#ifdef PARENTAL_CONTROL
memoryID const managedMemory::root = 1;
memoryID managedMemory::parent = managedMemory::root;
bool managedMemory::threadSynced = false;
pthread_mutex_t managedMemory::parentalMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t managedMemory::parentalCond = PTHREAD_COND_INITIALIZER;
pthread_t managedMemory::creatingThread = 0;
#endif



pthread_mutex_t managedMemory::stateChangeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t managedMemory::swappingCond = PTHREAD_COND_INITIALIZER;

managedMemory::managedMemory ( managedSwap *swap, global_bytesize size  )
{

    memory_max = size;
    memoryAlignment = swap->getMemoryAlignment();
    previousManager = defaultManager;
    defaultManager = this;
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = mmalloc ( 0 );                //Create root element.
    chunk->status = MEM_ROOT;
#endif
    this->swap = swap;
    if ( !swap ) {
        throw incompleteSetupException ( "no swap manager defined" );
    }
#ifdef SWAPSTATS
    signal ( SIGUSR1, sigswapstats );

#ifdef LOGSTATS
    if ( ! Timer::isRunning() ) {
        Timer::startTimer ( 0L, 100000000L );
    }
#endif
#endif
}

managedMemory::~managedMemory()
{
#ifdef SWAPSTATS
#ifdef LOGSTATS
    if ( previousManager == NULL ) {
        Timer::stopTimer();

        signal ( SIGUSR1, SIG_IGN );
    }

#endif
#endif

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
    pthread_mutex_lock ( &stateChangeMutex );
    if ( size > memory_max ) {
        memory_max = size;
        pthread_mutex_unlock ( &stateChangeMutex );
        return true;
    } else {
        if ( size < memory_used ) {

            //Try to swap out as much memory as needed:
            global_bytesize tobefreed = ( memory_used - size );
            if ( swapOut ( tobefreed ) != ERR_SUCCESS ) {
                pthread_mutex_unlock ( &stateChangeMutex );
                return false;
            }

            memory_max = size;
            swapOut ( 0 ); // Swap out further to adapt to new memory limits. This must not necessarily succeed.
            pthread_mutex_unlock ( &stateChangeMutex );
            return true;
        } else {
            memory_max = size;
            pthread_mutex_unlock ( &stateChangeMutex );
            return true;
        }
    }
    pthread_mutex_unlock ( &stateChangeMutex );
    return false;                                             //Resetting this is not implemented yet.
}

bool managedMemory::setOutOfSwapIsFatal ( bool fatal )
{
    bool old = outOfSwapIsFatal;
    outOfSwapIsFatal = fatal;
    return old;
}


bool managedMemory::ensureEnoughSpace ( global_bytesize sizereq, managedMemoryChunk *orisSwappedin )
{
    while ( sizereq + memory_used > memory_max ) {
        if ( orisSwappedin && ( orisSwappedin->status & MEM_ALLOCATED || orisSwappedin->status == MEM_SWAPIN ) ) {
            return true;
        }

        if ( sizereq + memory_used - memory_tobefreed > memory_max ) {
            managedMemory::swapErrorCode err = swapOut ( sizereq - ( memory_max - memory_used ) - memory_tobefreed );
            if (  err != ERR_SUCCESS ) { //Execute swapOut in protected context
                //We did not manage to swap out our stuff right away.
                if ( memory_tobefreed == 0 ) { //If other memory is to be freed, perhaps other threads may continue?
                    swap->cleanupCachedElements();
                    if ( outOfSwapIsFatal ) { //throw if user wants us to, otherwise wait indefinitely (ram-deadlock)
                        pthread_mutex_unlock ( &stateChangeMutex );
                        switch ( err ) {
                        case ERR_MORETHANTOTALRAM:
                            Throw ( memoryException ( "Could not swap memory: Object size requested is bigger than actual RAM limits" ) );
                        case ERR_NOTENOUGHCANDIDATES:
                            Throw ( memoryException ( "Could not swap memory: Too much used elements to make room" ) );
                        case ERR_SWAPFULL:
                            Throw ( memoryException ( "Could not swap memory: Swap is unrecoverably full" ) );
                        default:
                            Throw ( memoryException ( "Could not swap memory, I don't know why" ) );

                        }


                    }
                }
            }

        }
        if ( sizereq + memory_used > memory_max ) {
            waitForAIO();
        }
    }
    if ( orisSwappedin && ( orisSwappedin->status & MEM_ALLOCATED || orisSwappedin->status == MEM_SWAPIN ) ) {
        return true;
    }
    return false;
}


managedMemoryChunk *managedMemory::mmalloc ( global_bytesize sizereq )
{
    pthread_mutex_lock ( &stateChangeMutex );
    sizereq += sizereq % memoryAlignment == 0 ? 0 : memoryAlignment - sizereq % memoryAlignment; //f**k memoryAlignment
    ensureEnoughSpace ( sizereq );

    memory_used += sizereq;

    //We are left with enough free space to malloc.
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = new managedMemoryChunk ( parent, memID_pace++ );
#else
    managedMemoryChunk *chunk = new managedMemoryChunk ( memID_pace++ );
#endif
    chunk->status = MEM_ALLOCATED;
    chunk->size = sizereq;
    chunk->swapBuf = NULL;
#ifdef PARENTAL_CONTROL
    chunk->child = invalid;
    chunk->parent = parent;
    if ( chunk->id == root ) {                                //We're inserting root elem.

        chunk->next = invalid;
        chunk->schedBuf = NULL;
    } else {
#endif
        //Register this chunk in swapping logic:
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
        chunk->locPtr = _mm_malloc ( sizereq , memoryAlignment );
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

bool managedMemory::prepareUse ( managedMemoryChunk &chunk, bool acquireLock )
{
    if ( acquireLock ) {
        pthread_mutex_lock ( &stateChangeMutex );
    }
    ++chunk.useCnt;//This protects element from being swapped out by somebody else if it was swapped in.
    switch ( chunk.status ) {
    case MEM_SWAPOUT: // Object is about to be swapped out.
        waitForSwapout ( chunk, true );
    case MEM_SWAPPED:
        if ( !swapIn ( chunk ) ) {
            if ( acquireLock ) {
                pthread_mutex_unlock ( &stateChangeMutex );
            }
            return false;
        }
#ifdef SWAPSTATS
        ++swap_misses;
        --swap_hits;
#endif
    default:
        ;
    }
    if ( acquireLock ) {
        pthread_mutex_unlock ( &stateChangeMutex );
    }
    return true;

}


bool managedMemory::setUse ( managedMemoryChunk &chunk, bool writeAccess = false )
{
    pthread_mutex_lock ( &stateChangeMutex );
    ++chunk.useCnt;//This protects element from being swapped out by somebody else if it was swapped in.
    switch ( chunk.status ) {
    case MEM_SWAPOUT: // Object is about to be swapped out.
    case MEM_SWAPPED:
        prepareUse ( chunk, false );
        --chunk.useCnt;
    case MEM_SWAPIN: // Wait for object to appear
        if ( !waitForSwapin ( chunk, true ) ) {
            if ( ! ( chunk.status & MEM_ALLOCATED ) ) {
                pthread_mutex_unlock ( &stateChangeMutex );
                return false;
            }
        }
    case MEM_ALLOCATED:
        chunk.status = MEM_ALLOCATED_INUSE_READ;
    case MEM_ALLOCATED_INUSE:
    case MEM_ALLOCATED_INUSE_READ:
        if ( writeAccess ) {
            chunk.status = MEM_ALLOCATED_INUSE_WRITE;
            swap->invalidateCacheFor ( chunk );
        }
    case MEM_ALLOCATED_INUSE_WRITE:

#ifdef SWAPSTATS
        ++swap_hits;
#endif
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

    throw unfinishedCodeException ( "TODO: Locking, synchronizing, and everything" );
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
        signalSwappingCond();//Unsetting use may trigger different possible swapouts.
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
    if ( pthread_mutex_trylock ( &parentalMutex ) == 0 ) {
        pthread_mutex_unlock ( &parentalMutex );
    } else {
        if ( pthread_equal ( pthread_self(), creatingThread ) ) {
            pthread_mutex_unlock ( &parentalMutex );
        }
    }
#endif
    throw e;
}



void managedMemory::mfree ( memoryID id, bool inCleanup )
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
        if ( !inCleanup ) {
            schedulerDelete ( *chunk );
        }
        if ( chunk->status == MEM_ALLOCATED ) {
            _mm_free ( chunk->locPtr );
            memory_used -= chunk->size ;
        }
        if ( chunk->swapBuf ) {
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
    if ( !inCleanup ) {
        schedulerDelete ( *chunk );
    }
    if ( chunk->status == MEM_ALLOCATED ) {
        _mm_free ( chunk->locPtr );
        memory_used -= chunk->size ;
    }
    if ( chunk->swapBuf ) {
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
        printf ( "(%d : size %lu Bytes, %s, ", current->id, current->size, current->preemptiveLoaded ? "P" : "" );
        switch ( current->status ) {
        case MEM_ROOT:
            printf ( "Root Element" );
            break;
        case MEM_SWAPIN:
            printf ( "Swapping in" );
            break;
        case MEM_SWAPOUT:
            printf ( "Swapping out" );
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
        mfree ( oldchunk->id, true );
        oldchunk = next;
    } while ( 1 == 1 );
    mfree ( oldchunk->id, true );
}
#else
///\note This function does not preserve correct deallocation order in class hierarchies, as such, it is not a valid garbage collector.

void managedMemory::linearMfree()
{
    if ( memChunks.size() == 0 ) {
        return;
    }
    auto it = memChunks.begin();
    memoryID old;
    while ( it != memChunks.end() ) {
        old = it->first;
        ++it;
        mfree ( old , true );
    }
}

#endif
managedMemoryChunk &managedMemory::resolveMemChunk ( const memoryID &id )
{
    return *memChunks[id];
}

#ifdef SWAPSTATS
void managedMemory::printSwapstats() const
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

#define SAFESWAP(func) (defaultManager->swap != NULL ? defaultManager->swap->func : 0lu)

void managedMemory::sigswapstats ( int )
{
    if ( defaultManager == NULL ) {
        return;
    }

    global_bytesize usedSwap = SAFESWAP ( getUsedSwap() );
    global_bytesize totalSwap = SAFESWAP ( getSwapSize() );

#ifdef LOGSTATS
    if ( firstLog ) {
        fprintf ( managedMemory::logFile, "#Time [ms]\tPrep for swap out [B] \tSwapped out [B]\tSwapped out last [B]\tPrep for swap in [B] \tSwapped in [B]\tSwapped in last [B]\tHits / Miss\tMemory Used [B]\t\
                  Memory Used\tSwap Used [B]\tSwap Used\n" );
        firstLog = false;
    }
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds> ( std::chrono::high_resolution_clock::now().time_since_epoch() ).count();
    fprintf ( logFile, "%ld\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%e\t%lu\t%e\t%lu\t%e\n",
              now,
              defaultManager->swap_out_scheduled_bytes,
              defaultManager->swap_out_bytes,
              defaultManager->swap_out_bytes - defaultManager->swap_out_bytes_last,
              defaultManager->swap_in_scheduled_bytes,
              defaultManager->swap_in_bytes,
              defaultManager->swap_in_bytes - defaultManager->swap_in_bytes_last,
              ( double ) defaultManager->swap_hits / defaultManager->swap_misses,
              defaultManager->memory_used,
              ( double ) defaultManager->memory_used / defaultManager->memory_max,
              usedSwap,
              ( double ) usedSwap / totalSwap );
    fflush ( logFile );
#else
    printf ( "%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%e\t%lu\t%e\t%lu\t%e\n",
             defaultManager->swap_out_scheduled_bytes,
             defaultManager->swap_out_bytes,
             defaultManager->swap_out_bytes - defaultManager->swap_out_bytes_last,
             defaultManager->swap_in_scheduled_bytes,
             defaultManager->swap_in_bytes,
             defaultManager->swap_in_bytes - defaultManager->swap_in_bytes_last,
             ( double ) defaultManager->swap_hits / defaultManager->swap_misses,
             defaultManager->memory_used,
             ( double ) defaultManager->memory_used / defaultManager->memory_max,
             usedSwap,
             ( double ) usedSwap / totalSwap );
#endif
    defaultManager->swap_out_bytes_last = defaultManager->swap_out_bytes;
    defaultManager->swap_in_bytes_last = defaultManager->swap_in_bytes;
}
#endif

void managedMemory::versionInfo()
{

#ifdef SWAPSTATS
    const char *swapstats = "with swapstats";
#else
    const char *swapstats = "without swapstats";
#endif
#ifdef LOGSTATS
    const char *logstats = "with logstats";
#else
    const char *logstats = "Without logstats";
#endif
#ifdef PARENTAL_CONTROL
    const char *parentalcontrol = "with parental control";
#else
    const char *parentalcontrol = "without parental_control";
#endif

    infomsgf ( "compiled from %s\n\ton %s at %s\n\
                \t%s , %s , %s\n\
    \n \t git diff\n%s\n", gitCommit, __DATE__, __TIME__, swapstats, logstats, parentalcontrol, gitDiff );


}

void managedMemory::signalSwappingCond()
{
    pthread_cond_broadcast ( &swappingCond );
}

void managedMemory::waitForAIO()
{
    if ( swap->checkForAIO() ) { //Some AIO has arrived...
        return;
    }
    //Some other thread waits for us, we just linger around to see the result:
    pthread_cond_wait ( &swappingCond, &stateChangeMutex );
}


bool managedMemory::waitForSwapin ( managedMemoryChunk &chunk, bool keepSwapLock )
{
    if ( chunk.status == MEM_SWAPOUT ) { //Chunk is about to be swapped out...
        if ( !keepSwapLock ) {
            pthread_mutex_unlock ( &stateChangeMutex );
        }
        return false;
    }
    while ( ! ( chunk.status & MEM_ALLOCATED ) ) {
        waitForAIO();
    }
    if ( !keepSwapLock ) {
        pthread_mutex_unlock ( &stateChangeMutex );
    }
    return true;
}


bool managedMemory::waitForSwapout ( managedMemoryChunk &chunk, bool keepSwapLock )
{
    if ( chunk.status == MEM_SWAPIN ) { //We would wait indefinitely, as the chunk is not about to appear
        if ( !keepSwapLock ) {
            pthread_mutex_unlock ( &stateChangeMutex );
        }
        return false;
    }
    while ( ! ( chunk.status == MEM_SWAPPED ) ) {
        waitForAIO();
    }
    if ( !keepSwapLock ) {
        pthread_mutex_unlock ( &stateChangeMutex );
    }
    return true;
}


}

void rambrain::managedMemory::claimUsageof ( rambrain::global_bytesize bytes, bool rambytes, bool used )
{
    if ( rambytes ) {
        memory_used += used ? bytes : - bytes ;
    } else {
        memory_swapped += used ? bytes : -bytes ;
    }

}

void rambrain::managedMemory::claimTobefreed ( rambrain::global_bytesize bytes, bool tobefreed )
{
    memory_tobefreed += tobefreed ? bytes : -bytes;
}





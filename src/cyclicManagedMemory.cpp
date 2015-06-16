#include "cyclicManagedMemory.h"
#include "common.h"
#include "exceptions.h"
#include "membrain_atomics.h"
#include "managedSwap.h"
#include <pthread.h>
// #define VERYVERBOSE

#ifdef VERYVERBOSE
#define VERBOSEPRINT(x) printf("\n<--%s\n",x);printCycle();printf("\n%s---->\n",x);
#else
#define VERBOSEPRINT(x) ;
#endif

namespace membrain
{
pthread_mutex_t cyclicManagedMemory::cyclicTopoLock = PTHREAD_MUTEX_INITIALIZER;

cyclicManagedMemory::cyclicManagedMemory ( managedSwap *swap, global_bytesize size ) : managedMemory ( swap, size )
{
}


void cyclicManagedMemory::schedulerRegister ( managedMemoryChunk &chunk )
{

    cyclicAtime *neu = new cyclicAtime;

    //Couple chunk to atime and vice versa:
    neu->chunk = &chunk;
    chunk.schedBuf = ( void * ) neu;
    pthread_mutex_lock ( &cyclicTopoLock );
    if ( active == NULL ) { // We're inserting first one
        neu->prev = neu->next = neu;
        counterActive = neu;
    } else { //We're inserting somewhen.
        VERBOSEPRINT ( "registerbegin" );
        cyclicAtime *before = active->prev;
        cyclicAtime *after = active;
        MUTUAL_CONNECT ( before, neu );
        MUTUAL_CONNECT ( neu, after );
    }
    if ( counterActive == active && counterActive->chunk->status == MEM_SWAPPED ) {
        counterActive = neu;
    }
    active = neu;
    pthread_mutex_unlock ( &cyclicTopoLock );
}



void cyclicManagedMemory::schedulerDelete ( managedMemoryChunk &chunk )
{
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;
    //Memory counting for what we account for:
    if ( chunk.status == MEM_SWAPPED || chunk.status == MEM_SWAPOUT ) {

    } else if ( chunk.preemptiveLoaded ) {
        membrain_atomic_sub_fetch ( &preemptiveBytes, chunk.size );
        chunk.preemptiveLoaded = false;
    }

    pthread_mutex_lock ( &cyclicTopoLock );
    //Hook out element:
    if ( element->next == element ) {
        active = NULL;
        counterActive = NULL;
        delete element;
        pthread_mutex_unlock ( &cyclicTopoLock );
        return;

    }

    if ( element->next == element->prev ) {
        cyclicAtime *remaining = element->next;
        remaining->next = remaining->prev = remaining;
    } else {
        element->next->prev = element->prev;
        element->prev->next = element->next;
    }
    if ( active == element ) {
        active = element->next;
    }

    if ( counterActive == element ) {
        counterActive = element->prev;
    }

    delete element;
    pthread_mutex_unlock ( &cyclicTopoLock );
}

//Touch happens automatically after use, create, swapIn
bool cyclicManagedMemory::touch ( managedMemoryChunk &chunk )
{
    pthread_mutex_lock ( &cyclicTopoLock );
    if ( chunk.preemptiveLoaded ) { //This chunk was preemptively loaded
        membrain_atomic_sub_fetch ( &preemptiveBytes, chunk.size );
        chunk.preemptiveLoaded = false;
    }

    //Put this object to begin of loop:
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;

    if ( active == element ) {
        pthread_mutex_unlock ( &cyclicTopoLock );
        return true;
    }

    if ( counterActive == element ) {
        counterActive = counterActive->prev;
    }

    if ( active->prev == element ) {
        active = element;
        pthread_mutex_unlock ( &cyclicTopoLock );
        return true;
    };



    if ( active->next == element ) {
        cyclicAtime *before = active->prev;
        cyclicAtime *after = element->next;
        MUTUAL_CONNECT ( before, element );
        MUTUAL_CONNECT ( element, active );
        MUTUAL_CONNECT ( active, after );
        active = element;
        pthread_mutex_unlock ( &cyclicTopoLock );
        return true;
    }

    cyclicAtime *oldplace_before = element->prev;
    cyclicAtime *oldplace_after = element->next;
    cyclicAtime *before = active->prev;
    cyclicAtime *after = active;

    MUTUAL_CONNECT ( oldplace_before, oldplace_after );
    MUTUAL_CONNECT ( before, element );
    MUTUAL_CONNECT ( element, after );
    active = element;
    pthread_mutex_unlock ( &cyclicTopoLock );
    return true;
}

bool cyclicManagedMemory::setPreemptiveLoading ( bool preemptive )
{
    bool old = preemtiveSwapIn;
    preemtiveSwapIn = preemptive;
    return old;
}
void cyclicManagedMemory::printMemUsage()
{
    global_bytesize claimed_use = swap->getUsedSwap();
    fprintf ( stderr, "%lu\t%lu=%lu\t%lu\n", memory_used, claimed_use, memory_swapped, preemptiveBytes );
}


bool cyclicManagedMemory::swapIn ( managedMemoryChunk &chunk )
{
    VERBOSEPRINT ( "swapInEntry" );
    if ( chunk.status & MEM_ALLOCATED || chunk.status == MEM_SWAPIN ) {
        return true;
    }

    // We use the old border to ensure that sth is not swapped in again that was just swapped out.
    cyclicAtime *oldBorder = counterActive;


    global_bytesize actual_obj_size = chunk.size;
    //We want to read in what the user requested plus fill up the preemptive area with opportune guesses

    bool preemptiveAutoon = swap->getFreeSwap() / swap->getSwapSize() > preemptiveTurnoffFraction;

    if ( preemtiveSwapIn && preemptiveAutoon ) {
        global_bytesize targetReadinVol = actual_obj_size + ( swapInFrac - swapOutFrac ) * memory_max - preemptiveBytes;
        //fprintf(stderr,"%u %u %u %u %u\n",memory_used,preemptiveBytes,actual_obj_size,targetReadinVol,0);
        //Swap out if we want to read in more than what we thought:
        if ( targetReadinVol + memory_used > memory_max ) {
            global_bytesize targetSwapoutVol = actual_obj_size + ( 1. - swapOutFrac ) * memory_max - preemptiveBytes;
            swapErrorCode err = swapOut ( targetSwapoutVol );
            if ( err != ERR_SUCCESS ) {
                bool alreadyThere = ensureEnoughSpaceAndLockTopo ( actual_obj_size, &chunk );
                if ( alreadyThere ) {
                    waitForSwapin ( chunk, true );
                    return true;
                }
                targetReadinVol = actual_obj_size;
            }

        }
        VERBOSEPRINT ( "swapInAfterSwap" );
        cyclicAtime *readEl = ( cyclicAtime * ) chunk.schedBuf;
        cyclicAtime *cur = readEl;
        cyclicAtime *endSwapin = readEl;
        global_bytesize selectedReadinVol = 0;
        unsigned int numberSelected = 0;
        pthread_mutex_lock ( &cyclicTopoLock );
        do {
            if ( selectedReadinVol + cur->chunk->size > targetReadinVol ) {
                break;
            }
            cur->chunk->preemptiveLoaded = ( selectedReadinVol > 0 ? true : false );
            selectedReadinVol += cur->chunk->size;
            cur = cur->prev;
            ++numberSelected;
        } while ( cur != oldBorder );

        managedMemoryChunk *chunks[numberSelected];
        unsigned int n = 0;

        while ( readEl != cur ) {
            chunks[n++] = readEl->chunk;
            readEl = readEl->prev;
        };
        if ( ( swap->swapIn ( chunks, numberSelected ) != selectedReadinVol ) ) {
            return Throw ( memoryException ( "managedSwap failed to swap in :-(" ) );

        } else {
            cyclicAtime *beginSwapin = readEl->next;
            cyclicAtime *oldafter = endSwapin->next;
            if ( endSwapin != active ) { //swapped in element is already 'active' when all others have been swapped.
                if ( oldafter != active && beginSwapin != active ) {
                    ///\todo  Implement this for <3 elements
                    cyclicAtime *oldbefore = readEl;
                    cyclicAtime *before = active->prev;
                    MUTUAL_CONNECT ( oldbefore, oldafter );
                    MUTUAL_CONNECT ( endSwapin, active );
                    MUTUAL_CONNECT ( before, beginSwapin );
                }
                if ( counterActive == active && counterActive->chunk->status == MEM_SWAPPED ) {
                    counterActive = endSwapin;
                }
                active = endSwapin;
            }
            pthread_mutex_unlock ( &cyclicTopoLock );
            preemptiveBytes += selectedReadinVol - actual_obj_size;

#ifdef SWAPSTATS
            swap_in_bytes += selectedReadinVol;
            n_swap_in += 1;
#endif
            VERBOSEPRINT ( "swapInBeforeReturn" );
            waitForSwapin ( chunk, true );
            return true;
        }
    } else {
        bool alreadyThere = ensureEnoughSpaceAndLockTopo ( actual_obj_size, &chunk );
        if ( alreadyThere ) {
            waitForSwapin ( chunk, true );
            return true;
        }


        //We have to check wether the block is still swapped or not
        if ( swap->swapIn ( &chunk ) == chunk.size ) {
            //Wait for object to be swapped in:
            waitForSwapin ( chunk, true );
            touch ( chunk );
            pthread_mutex_lock ( &cyclicTopoLock );
            if ( counterActive->chunk->status == MEM_SWAPPED ) {
                counterActive = active;
            }
            pthread_mutex_unlock ( &cyclicTopoLock );
#ifdef SWAPSTATS
            swap_in_bytes += chunk.size;
            n_swap_in += 1;
#endif
            return true;
        } else {
            //Unlock mutex under which we were called, as we'll be throwing...
            pthread_mutex_unlock ( &stateChangeMutex );
            return Throw ( memoryException ( "Could not swap in an element." ) );
        };
    }

}

//Idea: swap out more than required, as the free space may be filled with premptive swap-ins

bool cyclicManagedMemory::checkCycle()
{
    pthread_mutex_lock ( &cyclicTopoLock );
#ifdef PARENTAL_CONTROL
    unsigned int no_reg = memChunks.size() - 1;
#else
    unsigned int no_reg = memChunks.size();
#endif
    unsigned int encountered = 0;
    cyclicAtime *cur = active;
    cyclicAtime *oldcur;

    if ( !cur ) {
        pthread_mutex_unlock ( &cyclicTopoLock );
        if ( no_reg == 0 && counterActive == NULL ) {
            return true;
        } else {
            return false;
        }
    }

    bool inActiveOnlySection = ( active->chunk->status == MEM_SWAPPED || active->chunk->status == MEM_SWAPOUT ? false : true );
    bool inSwapsection = true;
    do {
        ++encountered;
        oldcur = cur;
        cur = cur->next;
        if ( oldcur != cur->prev ) {
            errmsg ( "Mutual connecion failure" );
            pthread_mutex_unlock ( &cyclicTopoLock );
            return false;
        }

        if ( inActiveOnlySection ) {
            if ( oldcur->chunk->status == MEM_SWAPPED || oldcur->chunk->status == MEM_SWAPOUT ) {
                errmsg ( "Swapped elements in active section!" );
                pthread_mutex_unlock ( &cyclicTopoLock );
                return false;
            }
        } else {
            if ( oldcur->chunk->status == MEM_SWAPPED && !inSwapsection ) {
                errmsg ( "Isolated swapped element block not tracked by counterActive found!" );
                pthread_mutex_unlock ( &cyclicTopoLock );
                return false;
            }
            if ( oldcur->chunk->status != MEM_SWAPPED && oldcur->chunk->status != MEM_SWAPOUT ) {
                inSwapsection = false;
            }
        }

        if ( oldcur == counterActive ) {
            inActiveOnlySection = false;
        }

    } while ( cur != active );
    pthread_mutex_unlock ( &cyclicTopoLock );
    if ( encountered != no_reg ) {
        errmsgf ( "Not all elements accessed. %d expected,  %d encountered", no_reg, encountered );
        return false;
    } else {
        return true;
    }

}

void cyclicManagedMemory::printCycle()
{
    cyclicAtime *atime = active;
    checkCycle();
    if ( memChunks.size() <= 0 ) {
        infomsg ( "No objects." );
        return;
    }
    printf ( "%d (%s)<-counterActive\n", counterActive->chunk->id, ( counterActive->chunk->preemptiveLoaded ? "p" : " " ) );
    printf ( "%d => %d => %d\n", counterActive->prev->chunk->id, counterActive->chunk->id, counterActive->next->chunk->id );
    printf ( "%d (%s)<-active\n", active->chunk->id, ( active->chunk->preemptiveLoaded ? "p" : " " ) );
    printf ( "%d => %d => %d\n", active->prev->chunk->id, active->chunk->id, active->next->chunk->id );
    printf ( "\n" );
    do {
        char  status[2];
        status[1] = 0x00;
        switch ( atime->chunk->status ) {
        case MEM_ALLOCATED:
            status[0] = 'A';
            break;
        case MEM_SWAPIN:
            status[0] = 'I';
            break;
        case MEM_SWAPOUT:
            status[0] = 'O';
            break;
        case MEM_SWAPPED:
            status[0] = 'S';
            break;
        case MEM_ALLOCATED_INUSE:
            status[0] = '?';
            break;
        case MEM_ALLOCATED_INUSE_WRITE:
            status[0] = 'W';
            break;
        case MEM_ALLOCATED_INUSE_READ:
            status[0] = 'U';
            break;
        case MEM_ROOT:
            status[0] = 'R';
            break;
        }
        if ( atime == counterActive ) {
            printf ( "%d (%s) %s <-counterActive\n", atime->chunk->id, ( atime->chunk->preemptiveLoaded ? "p" : " " ), status );
        } else {
            printf ( "%d (%s) %s \n", atime->chunk->id, ( atime->chunk->preemptiveLoaded ? "p" : " " ), status );
        }
        atime = atime->next;
    } while ( atime != active );
    printf ( "\n" );
    printf ( "\n" );
}

cyclicManagedMemory::swapErrorCode cyclicManagedMemory::swapOut ( membrain::global_bytesize min_size )
{
    pthread_mutex_lock ( &cyclicTopoLock );
    if ( counterActive == 0 ) {
        pthread_mutex_unlock ( &stateChangeMutex );
        pthread_mutex_unlock ( &cyclicTopoLock );
        Throw ( memoryException ( "I can't swap out anything if there's nothing to swap out." ) );
    }
    VERBOSEPRINT ( "swapOutEntry" );
    if ( min_size > memory_max ) {
        pthread_mutex_unlock ( &cyclicTopoLock );
        return ERR_MORETHANTOTALRAM;
    }
    global_bytesize swap_free = swap->getFreeSwap();
    if ( min_size > swap_free ) {
        pthread_mutex_unlock ( &cyclicTopoLock );
        return ERR_SWAPFULL;
    }

    global_bytesize mem_alloc_max = memory_max * swapOutFrac; //<- This is target size
    global_bytesize mem_swap_min = memory_used > mem_alloc_max ? memory_used - mem_alloc_max : 0;
    global_bytesize mem_swap = mem_swap_min < min_size ? min_size : mem_swap_min;

    mem_swap = mem_swap > swap_free ? min_size : mem_swap;

    cyclicAtime *fromPos = counterActive;
    cyclicAtime *countPos = counterActive;
    global_bytesize unload_size = 0;
    global_bytesize unload_size2 = 0;
    unsigned int passed = 0, unload = 0;
    while ( unload_size < mem_swap ) {
        ++passed;
        if ( countPos->chunk->status == MEM_ALLOCATED && ( unload_size + countPos->chunk->size <= swap_free ) ) {
            if ( countPos->chunk->size + unload_size <= swap_free ) {
                unload_size += countPos->chunk->size;
                ++unload;
            }
        }
        countPos = countPos->prev;
        if ( fromPos == countPos ) {
            break;
        }

    }
    if ( unload_size == 0 ) {
        pthread_mutex_unlock ( &cyclicTopoLock );
        return ERR_NOTENOUGHCANDIDATES;
    }

    managedMemoryChunk *unloadlist[unload];
    managedMemoryChunk **unloadElem = unloadlist;

    do {
        if ( fromPos->chunk->status == MEM_ALLOCATED && ( unload_size2 + fromPos->chunk->size <= swap_free ) ) {
            if ( fromPos->chunk->size + unload_size2 <= swap_free ) {
                *unloadElem = fromPos->chunk;
                ++unloadElem;
                unload_size2 += fromPos->chunk->size;
                if ( fromPos == active ) {
                    active = fromPos->prev;
                }
            }
        }
        fromPos = fromPos->prev;
    } while ( fromPos != countPos );
    global_bytesize real_unloaded = swap->swapOut ( unloadlist, unload );
    bool swapSuccess = ( real_unloaded == unload_size2 ) ;
    if ( !swapSuccess ) {
        if ( real_unloaded == 0 ) {
            pthread_mutex_unlock ( &cyclicTopoLock );
            return ERR_NOTENOUGHCANDIDATES;
        }
    }
    fromPos = counterActive;
    cyclicAtime *moveEnd, *cleanFrom;
    moveEnd = NULL;

    bool inSwappedSection = ( fromPos->chunk->status == MEM_SWAPPED | fromPos->chunk->status == MEM_SWAPOUT );
    bool doRoundtrip = fromPos == countPos;


    cleanFrom = counterActive->next;

    ///\todo Implement this for less than 3 elements!
    while ( fromPos != countPos || doRoundtrip ) {
        doRoundtrip = false;
        if ( inSwappedSection ) {
            if ( fromPos->chunk->status != MEM_SWAPPED && fromPos->chunk->status != MEM_SWAPOUT ) {
                inSwappedSection = false;
                if ( moveEnd ) {
                    //  xxxxxxxxxxoooooooxxxxxxooooooo
                    //  ------A--><--B--><-C--><--D
                    // Change order from A-B-C-D to A-C-B-D

                    cyclicAtime *endNonswap = fromPos; //A
                    cyclicAtime *startIsoswap = fromPos->next; //B
                    cyclicAtime *endIsoswap = moveEnd; //B
                    cyclicAtime *startIsoNonswap = moveEnd->next;//C
                    cyclicAtime *endIsoNonswap = cleanFrom->prev;//C
                    cyclicAtime *startClean = cleanFrom;//D
                    //A-C:
                    MUTUAL_CONNECT ( endNonswap, startIsoNonswap );
                    MUTUAL_CONNECT ( endIsoNonswap, startIsoswap );
                    MUTUAL_CONNECT ( endIsoswap, startClean );
                    cleanFrom = startIsoswap;
                    moveEnd = NULL;
                }
            } else {
                if ( !moveEnd ) {
                    cleanFrom = fromPos;
                }
            }
        } else {
            if ( fromPos->chunk->status == MEM_SWAPPED || fromPos->chunk->status == MEM_SWAPOUT ) {
                inSwappedSection = true;
                if ( moveEnd == NULL ) {
                    moveEnd = fromPos;
                }
            }
        }
        fromPos = fromPos->prev;
    }
    if ( moveEnd ) {
        //  xxxxxxxxxxoooooooxxxxxxooooooo
        //  ------A--><--B--><-C--><--D
        // Change order from A-B-C-D to A-C-B-D
        cyclicAtime *endNonswap = fromPos; //A
        cyclicAtime *startIsoswap = fromPos->next; //B
        cyclicAtime *endIsoswap = moveEnd; //B
        cyclicAtime *startIsoNonswap = moveEnd->next;//C
        cyclicAtime *endIsoNonswap = cleanFrom->prev;//C
        cyclicAtime *startClean = cleanFrom;//D
        //A-C:
        MUTUAL_CONNECT ( endNonswap, startIsoNonswap );
        MUTUAL_CONNECT ( endIsoNonswap, startIsoswap );
        MUTUAL_CONNECT ( endIsoswap, startClean );
        cleanFrom = startIsoswap;
        moveEnd = NULL;
    }
    counterActive = cleanFrom->prev;
    pthread_mutex_unlock ( &cyclicTopoLock );
    VERBOSEPRINT ( "swapOutReturn" );
    if ( swapSuccess ) {
#ifdef SWAPSTATS
        swap_out_bytes += unload_size;
        n_swap_out += 1;
#endif
        return ERR_SUCCESS;
    } else {
        if ( real_unloaded > min_size ) {
            return ERR_SUCCESS;
        } else {
            return ERR_NOTENOUGHCANDIDATES;
        }
    }
}

}

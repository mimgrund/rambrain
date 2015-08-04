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

#include "cyclicManagedMemory.h"
#include "common.h"
#include "exceptions.h"
#include "rambrain_atomics.h"
#include "managedSwap.h"
#include <pthread.h>
//#define VERYVERBOSE
//#define CYCLIC_VERBOSE_DBG

#ifdef VERYVERBOSE
#define VERBOSEPRINT(x) printf("\n<--%s\n",x);printCycle();printf("\n%s---->\n",x);
#else
#define VERBOSEPRINT(x) ;
#endif



namespace rambrain
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
    pthread_mutex_lock ( &cyclicTopoLock );
    if ( chunk.status == MEM_SWAPPED || chunk.status == MEM_SWAPOUT ) {

    } else if ( chunk.preemptiveLoaded ) {
        preemptiveBytes -= chunk.size;
        chunk.preemptiveLoaded = false;
    }


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
        preemptiveBytes -= chunk.size ;
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

bool cyclicManagedMemory::setPreemptiveUnloading ( bool preemptive )
{
    bool old = preemtiveSwapOut;
    preemtiveSwapOut = preemptive;
    return old;
}

void cyclicManagedMemory::printMemUsage() const
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

    bool preemptiveAutoon = ( ( double ) swap->getFreeSwap() ) / swap->getSwapSize() > preemptiveTurnoffFraction;

    if ( preemtiveSwapIn && preemptiveAutoon ) {
#ifdef VERYVERBOSE
        printf ( "Preemptive swapin\n" );
#endif
        global_bytesize targetReadinVol = actual_obj_size + ( swapInFrac - swapOutFrac ) * memory_max - preemptiveBytes;
        //fprintf(stderr,"%u %u %u %u %u\n",memory_used,preemptiveBytes,actual_obj_size,targetReadinVol,0);

        //Swap out if we want to read in more than what we thought:
        if ( targetReadinVol + memory_used > memory_max ) {
            global_bytesize targetSwapoutVol = actual_obj_size + ( 1. - swapOutFrac ) * memory_max - preemptiveBytes;
            swapErrorCode err = swapOut ( targetSwapoutVol );
            if ( err != ERR_SUCCESS ) {
                bool alreadyThere = ensureEnoughSpace ( actual_obj_size, &chunk );
                if ( alreadyThere ) {
                    waitForSwapin ( chunk, true );
                    return true;
                }
                targetReadinVol = actual_obj_size;
            }
            targetReadinVol = targetSwapoutVol;
        }
        VERBOSEPRINT ( "swapInAfterSwap" );
        cyclicAtime *readEl = ( cyclicAtime * ) chunk.schedBuf;
        cyclicAtime *cur = readEl;
        cyclicAtime *endSwapin = readEl;
        global_bytesize selectedReadinVol = 0;
        unsigned int numberSelected = 0;
        pthread_mutex_lock ( &cyclicTopoLock );

        //Why do we not have to check for chunk's status?
        // Because, as we should load in a swapped element, we're in the swapped section,
        // which only contains swapped elements until counterActive is reached.
        do {
            cur->chunk->preemptiveLoaded = ( selectedReadinVol > 0 ? true : false );
            ++numberSelected;
            selectedReadinVol += cur->chunk->size;
            if ( selectedReadinVol >= targetReadinVol ) {
                cur = cur->prev;
                break;
            }
            cur = cur->prev;
        } while ( cur != oldBorder );

        managedMemoryChunk *chunks[numberSelected];
        unsigned int n = 0;

        do {
            chunks[n++] = readEl->chunk;
            readEl = readEl->prev;
        } while ( readEl != cur );
        global_bytesize swappedInBytes = swap->swapIn ( chunks, numberSelected );
        if ( (  swappedInBytes != selectedReadinVol ) ) {
            //Check if we at least have swapped in enough:
            if ( ! ( chunk.status & MEM_ALLOCATED || chunk.status == MEM_SWAPIN ) ) {
                return Throw ( memoryException ( "managedSwap failed to swap in :-(" ) );
            }

        }
        cur = cur->next;
        cyclicAtime *beginSwapin = readEl->next;
        cyclicAtime *oldafter = endSwapin->next;
        if ( endSwapin != active ) { //swapped in element is already 'active' when all others have been swapped.
            if ( oldafter != active && beginSwapin != active ) {
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

        preemptiveBytes += selectedReadinVol - actual_obj_size;

#ifdef SWAPSTATS
        swap_in_scheduled_bytes += selectedReadinVol;
        n_swap_in += 1;
#endif
        VERBOSEPRINT ( "swapInBeforeReturn" );
        pthread_mutex_unlock ( &cyclicTopoLock );
        return true;

    } else {
#ifdef VERYVERBOSE
        printf ( "Non-preemptive swapin\n" );
#endif
        bool alreadyThere = ensureEnoughSpace ( actual_obj_size, &chunk );
        if ( alreadyThere ) {
            return true;
        }


        //We have to check wether the block is still swapped or not
        if ( swap->swapIn ( &chunk ) == chunk.size ) {
            //Wait for object to be swapped in:
            touch ( chunk );
            pthread_mutex_lock ( &cyclicTopoLock );
            if ( counterActive->chunk->status == MEM_SWAPPED ) {
                counterActive = active;
            }

#ifdef SWAPSTATS
            swap_in_scheduled_bytes += chunk.size;
            n_swap_in += 1;
#endif
            pthread_mutex_unlock ( &cyclicTopoLock );
            return true;
        } else {
            //Unlock mutex under which we were called, as we'll be throwing...
            pthread_mutex_unlock ( &stateChangeMutex );
            return Throw ( memoryException ( "Could not swap in an element." ) );
        };
    }

}


void cyclicManagedMemory::untouch ( managedMemoryChunk &chunk )
{
    if ( !preemtiveSwapOut ) {
        return;
    }
    global_bytesize keep_free_for_user = ( 1. - swapInFrac ) * ( memory_max );
    global_bytesize total_preemptive_needed = preemtiveSwapIn ? ( swapInFrac - swapOutFrac ) * ( memory_max ) - preemptiveBytes : 0;
    //We also account for memory that is in the process of becoming free:
    global_bytesize currently_free = memory_max - memory_used + memory_tobefreed;
    global_bytesize desired_free = total_preemptive_needed + keep_free_for_user;
    if ( currently_free < desired_free ) {
        global_bytesize try_free = desired_free - currently_free;
        swapOut ( try_free );
    }
}


bool cyclicManagedMemory::checkCycle() const
{
    pthread_mutex_lock ( &stateChangeMutex );
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
        pthread_mutex_unlock ( &stateChangeMutex );
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
            pthread_mutex_unlock ( &stateChangeMutex );

            return false;
        }

        if ( inActiveOnlySection ) {
            if ( oldcur->chunk->status == MEM_SWAPPED || oldcur->chunk->status == MEM_SWAPOUT ) {
                errmsg ( "Swapped elements in active section!" );

                pthread_mutex_unlock ( &cyclicTopoLock );
                pthread_mutex_unlock ( &stateChangeMutex );
                return false;
            }
        } else {
            if ( oldcur->chunk->status == MEM_SWAPPED && !inSwapsection ) {
                errmsg ( "Isolated swapped element block not tracked by counterActive found!" );
                pthread_mutex_unlock ( &cyclicTopoLock );
                pthread_mutex_unlock ( &stateChangeMutex );
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
    pthread_mutex_unlock ( &stateChangeMutex );
    if ( encountered != no_reg ) {
        errmsgf ( "Not all elements accessed. %d expected,  %d encountered", no_reg, encountered );
        return false;
    } else {
        return true;
    }

}

void cyclicManagedMemory::printCycle() const
{
    cyclicAtime *atime = active;

    if ( memChunks.size() <= 0 ) {
        infomsg ( "No objects." );
        return;
    }
    printf ( "%lu (%s)<-counterActive\n", counterActive->chunk->id, ( counterActive->chunk->preemptiveLoaded ? "p" : " " ) );
    printf ( "%lu => %lu => %lu\n", counterActive->prev->chunk->id, counterActive->chunk->id, counterActive->next->chunk->id );
    printf ( "%lu (%s)<-active\n", active->chunk->id, ( active->chunk->preemptiveLoaded ? "p" : " " ) );
    printf ( "%lu => %lu => %lu\n", active->prev->chunk->id, active->chunk->id, active->next->chunk->id );
    printf ( "\n" );
    do {
        char  status[2];
        status[1] = 0x00;
        switch ( atime->chunk->status ) {
        case MEM_ALLOCATED:
            if ( atime->chunk->useCnt > 0 ) {
                status[0] = 'a';    //Small letters means that usage is already claimed.
            } else {
                status[0] = 'A';
            }
            break;
        case MEM_SWAPIN:
            if ( atime->chunk->useCnt > 0 ) {
                status[0] = 'i';    //Small letters means that usage is already claimed.
            } else {
                status[0] = 'I';
            }
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
            printf ( "%lu (%s) %s <-counterActive\t", atime->chunk->id, ( atime->chunk->preemptiveLoaded ? "p" : " " ), status );
        } else {
            printf ( "%lu (%s) %s \t", atime->chunk->id, ( atime->chunk->preemptiveLoaded ? "p" : " " ), status );
        }
        atime = atime->next;
    } while ( atime != active );
    printf ( "\n" );
    printf ( "\n" );
}

cyclicManagedMemory::swapErrorCode cyclicManagedMemory::swapOut ( rambrain::global_bytesize min_size )
{
#ifdef CYCLIC_VERBOSE_DBG
    printCycle();
#endif
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
    if ( swap_free < min_size ) {
        if ( !swap->extendSwapByPolicy ( min_size ) ) {
            pthread_mutex_unlock ( &cyclicTopoLock );
            return ERR_SWAPFULL;
        }
        swap_free = swap->getFreeSwap();
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
    unsigned int preemptiveBytesStill = preemptiveBytes;
    bool activeReached = false;
    while ( unload_size < mem_swap ) {
        ++passed;
        if ( countPos->chunk->status == MEM_ALLOCATED && ( unload_size + countPos->chunk->size <= swap_free ) ) {
            if ( countPos->chunk->size + unload_size <= swap_free ) {
                unload_size += countPos->chunk->size;
                ++unload;
#ifdef CYCLIC_VERBOSE_DBG
                printf ( "U(%d)\t", countPos->chunk->id );
#endif
            }
        }

        if ( active == countPos ) {
            activeReached = true;
        }
        if ( activeReached ) {
            preemptiveBytesStill -= countPos->chunk->preemptiveLoaded ? countPos->chunk->size : 0;
            if ( preemptiveBytesStill == 0 ) {
#ifdef CYCLIC_VERBOSE_DBG
                printf ( "emergency, once round!\n" );
#endif
                break;
            }
        }

        countPos = countPos->prev;
    }
    if ( unload_size == 0 ) {
        pthread_mutex_unlock ( &cyclicTopoLock );
        return ERR_NOTENOUGHCANDIDATES;
    }

    managedMemoryChunk *unloadlist[unload];
    managedMemoryChunk **unloadElem = unloadlist;
#ifdef CYCLIC_VERBOSE_DBG
    printf ( "active = %d\n", active->chunk->id );
#endif
    activeReached = false;
    preemptiveBytesStill = preemptiveBytes;
    while ( unload_size2 < mem_swap ) {
        ++passed;
        if ( fromPos->chunk->status == MEM_ALLOCATED && ( unload_size2 + fromPos->chunk->size <= swap_free ) ) {
            if ( fromPos->chunk->size + unload_size2 <= swap_free ) {
                unload_size2 += fromPos->chunk->size;
                *unloadElem = fromPos->chunk;
                ++unloadElem;
                if ( fromPos->chunk->preemptiveLoaded ) { //We had this chunk preemptive, but now have to swap out.
                    //This is a bit evil, as we will reload preemptive bytes when we've swapped them out.
                    ///@todo investigate if subtracting swapped out preemptive bytes is affecting performance (too much preemptive action possible)
                    fromPos->chunk->preemptiveLoaded = false;
                    preemptiveBytes -= fromPos->chunk->size;
                }
            }
        }

        if ( active == fromPos ) {
            activeReached = true;
        }
        if ( activeReached ) {
            preemptiveBytesStill -= fromPos->chunk->preemptiveLoaded ? fromPos->chunk->size : 0;
            if ( preemptiveBytesStill == 0 ) {
#ifdef CYCLIC_VERBOSE_DBG
                printf ( "emergency, once round!\n" );
#endif
                break;
            }
        }

        fromPos = fromPos->prev;
    }
    global_bytesize real_unloaded = swap->swapOut ( unloadlist, unload );
    bool swapSuccess = ( real_unloaded == unload_size2 ) ;
    if ( !swapSuccess ) {
        if ( real_unloaded == 0 ) {
            pthread_mutex_unlock ( &cyclicTopoLock );
            return ERR_NOTENOUGHCANDIDATES;
        }
    }
#ifdef CYCLIC_VERBOSE_DBG
    printf ( "active = %d\n", active->chunk->id );
#endif
    fromPos = counterActive;
    cyclicAtime *moveEnd, *cleanFrom;
    moveEnd = NULL;

    bool inSwappedSection = ( fromPos->chunk->status == MEM_SWAPPED || fromPos->chunk->status == MEM_SWAPOUT );
    bool doRoundtrip = fromPos == countPos;


    cleanFrom = counterActive->next;
#ifdef CYCLIC_VERBOSE_DBG
    printCycle();

    printf ( "Begin Movement until %d\n", countPos->chunk->id );
#endif
    while ( fromPos != countPos || doRoundtrip ) {
#ifdef CYCLIC_VERBOSE_DBG
        printf ( "at %d\t", fromPos->chunk->id );
#endif
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
#ifdef CYCLIC_VERBOSE_DBG
                    printf ( "----%d><%d-%d><%d-%d><%d---\n", endNonswap->chunk->id, startIsoswap->chunk->id, endIsoswap->chunk->id,
                             startIsoNonswap->chunk->id, endIsoNonswap->chunk->id, startClean->chunk->id );
#endif
                    //A-C:
                    MUTUAL_CONNECT ( endNonswap, startIsoNonswap );
                    MUTUAL_CONNECT ( endIsoNonswap, startIsoswap );
                    MUTUAL_CONNECT ( endIsoswap, startClean );
                    cleanFrom = startIsoswap;
                    moveEnd = NULL;
#ifdef CYCLIC_VERBOSE_DBG
                    printCycle();
#endif
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
#ifdef CYCLIC_VERBOSE_DBG
    printf ( "After loop at %d\n", fromPos->chunk->id );
#endif
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
#ifdef CYCLIC_VERBOSE_DBG
        printf ( "LAST----%d><%d-%d><%d-%d><%d---\n", endNonswap->chunk->id, startIsoswap->chunk->id, endIsoswap->chunk->id,
                 startIsoNonswap->chunk->id, endIsoNonswap->chunk->id, startClean->chunk->id );
#endif
        //A-C:
        MUTUAL_CONNECT ( endNonswap, startIsoNonswap );
        MUTUAL_CONNECT ( endIsoNonswap, startIsoswap );
        MUTUAL_CONNECT ( endIsoswap, startClean );
        cleanFrom = startIsoswap;
        moveEnd = NULL;
    }
    counterActive = cleanFrom->prev;
    while ( ! ( counterActive->chunk->status & MEM_ALLOCATED || counterActive->chunk->status == MEM_SWAPIN || counterActive == active ) ) {
        counterActive = counterActive->prev;
    }
    if ( ! ( active->chunk->status & MEM_ALLOCATED || active->chunk->status == MEM_SWAPIN ) ) { // We may have swapped out the first allocated element
        active = counterActive;
#ifdef CYCLIC_VERBOSE_DBG
        printf ( "had to move active", fromPos->chunk->id );
#endif
    }
    pthread_mutex_unlock ( &cyclicTopoLock );
#ifdef CYCLIC_VERBOSE_DBG
    printCycle();
#endif
    VERBOSEPRINT ( "swapOutReturn" );
    if ( swapSuccess ) {
#ifdef SWAPSTATS
        swap_out_scheduled_bytes += unload_size;
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

cyclicManagedMemory::~cyclicManagedMemory()
{
    auto it = memChunks.begin();
    while ( it != memChunks.end() ) {
        cyclicAtime *element = ( cyclicAtime * ) it->second->schedBuf;
        if ( element ) {
            delete element;
        }
        ++it;
    }
}

}


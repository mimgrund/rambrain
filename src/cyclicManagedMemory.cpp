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
#include <cmath>
//#define VERYVERBOSE





namespace rambrain
{
#ifdef VERYVERBOSE
global_bytesize min_elements = 0;

#define VERBOSEPRINT(x) printf("\n<--%s\n",x); if (memChunks.size()>=min_elements) printCycle();printf("\n%s---->\n",x);
#else
#define VERBOSEPRINT(x) ;
#endif

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
    rambrain_pthread_mutex_lock ( &cyclicTopoLock );
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
    rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
}



void cyclicManagedMemory::schedulerDelete ( managedMemoryChunk &chunk )
{
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;
    //Memory counting for what we account for:
    rambrain_pthread_mutex_lock ( &cyclicTopoLock );
    if ( chunk.status == MEM_SWAPPED || chunk.status == MEM_SWAPOUT ) {

    } else if ( chunk.preemptiveLoaded ) {
        preemptiveBytes -= chunk.size;
        chunk.preemptiveLoaded = false;
    }


    if ( preemptiveStart && &chunk == preemptiveStart->chunk ) {
        preemptiveStart = ( preemptiveStart->next == active ? NULL : preemptiveStart->next );
    }

    //Hook out element:
    if ( element->next == element ) {
        active = NULL;
        counterActive = NULL;
        delete element;
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
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
    rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
}

//Touch happens automatically after use, create, swapIn
bool cyclicManagedMemory::touch ( managedMemoryChunk &chunk )
{
    rambrain_pthread_mutex_lock ( &cyclicTopoLock );
    if ( chunk.preemptiveLoaded ) { //This chunk was preemptively loaded
        ++consecutivePreemptiveTransactions;
        preemptiveBytes -= chunk.size;

        chunk.preemptiveLoaded = false;
    } else {
        consecutivePreemptiveTransactions = 0;
    }
    // This can be the case even if chunk.preemptiveLoaded is false when we have just swapped in this one as active
    if ( preemptiveStart && ( preemptiveStart->chunk == &chunk ) ) {
        if ( preemptiveStart->next == active ) {
            preemptiveStart = NULL;
        } else {
            preemptiveStart = preemptiveStart->next;
        }
    }

    //Put this object to begin of loop:
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;

    if ( active == element ) {
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        return true;
    }

    if ( counterActive == element ) {
        counterActive = counterActive->prev;
    }

    if ( active->prev == element ) {
        active = element;
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        return true;
    };



    if ( active->next == element ) {
        cyclicAtime *before = active->prev;
        cyclicAtime *after = element->next;
        MUTUAL_CONNECT ( before, element );
        MUTUAL_CONNECT ( element, active );
        MUTUAL_CONNECT ( active, after );
        active = element;
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
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
    rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
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
void cyclicManagedMemory::insertBefore ( cyclicAtime *pos, cyclicManagedMemory::chain separated )
{
    if ( !separated.from ) {
        return;
    }
    cyclicAtime *before = pos->prev;
    cyclicAtime *after = pos;
    MUTUAL_CONNECT ( before, separated.from );
    MUTUAL_CONNECT ( separated.to, after );
}


cyclicManagedMemory::chain cyclicManagedMemory::filterChain ( chain &toFilter, const memoryStatus *separateStatus, bool *preemptiveLoaded )
{
    cyclicAtime *cur = toFilter.from;
    cyclicAtime *sepaStart = NULL;
    struct cyclicManagedMemory::chain separated = {NULL, NULL};

    //First, insert an element at end to cope with all types (cyclic, noncyclic) at once:
    cyclicAtime end;
    cyclicAtime *after = toFilter.to->next;
    cyclicAtime *before = toFilter.from->prev;
    bool cyclic = ( after == toFilter.from );
    cyclicAtime *firstValid = NULL, *lastValid = NULL;
    MUTUAL_CONNECT ( toFilter.to, ( &end ) );
    MUTUAL_CONNECT ( ( &end ), toFilter.from );

    do {
        //determine whether to separate this chunk:
        bool separateThis = false;
        for ( const memoryStatus *status = separateStatus; ( *status != MEM_ROOT ); ++status ) {
            if ( cur->chunk->status == *status ) {
                separateThis = true;
                goto afterchecks;
            }
        }
        if ( preemptiveLoaded ) {
            separateThis = ! ( cur->chunk->preemptiveLoaded ^ *preemptiveLoaded );
        }
afterchecks:
        if ( separateThis ) { // we should separate this element
            if ( !sepaStart ) { // if its the first to separate, lets mark this as start
                sepaStart = cur;
            }

        } else { // We should not separate this
            if ( !firstValid ) {
                firstValid = cur;
            }
            lastValid = cur;
            if ( sepaStart ) { //We had started separating something, so lets cut these out
                cyclicAtime *newTo = cur->prev;
                MUTUAL_CONNECT ( sepaStart->prev, cur );
                if ( separated.from == NULL ) { // first separated element
                    separated.from = sepaStart;

                } else { //We had previously separated elements, lets chain the end of the formerly separated to this one:
                    MUTUAL_CONNECT ( separated.to, sepaStart );
                }
                separated.to =  newTo;

                sepaStart = NULL;
            } else {
                //We are not in a separate region and do not have a chain to end, so nothing happens.
            }
        }
        cur = cur->next;
    } while ( cur != &end );

    //The cur==to element is by definition out of selection, thus we may close the separated area if it exists:
    if ( sepaStart ) { //We had started separating something, so lets cut these out
        cyclicAtime *newTo = cur->prev;
        MUTUAL_CONNECT ( sepaStart->prev, cur );
        if ( separated.from == NULL ) { // first separated element
            separated.from = sepaStart;

        } else { //We had previously separated elements, lets chain the end of the formerly separated to this one:
            MUTUAL_CONNECT ( separated.to, sepaStart );
        }
        separated.to = newTo;
    }
    toFilter = {firstValid, lastValid};
    if ( lastValid ) {
        if ( !cyclic ) {
            MUTUAL_CONNECT ( before, firstValid );
            MUTUAL_CONNECT ( lastValid, after );
        } else {
            MUTUAL_CONNECT ( lastValid, firstValid );
        }
    } else {
        if ( !cyclic ) {
            MUTUAL_CONNECT ( before, after );
        }
    }
    return separated;
}

void cyclicManagedMemory::decay ( global_bytesize bytes )
{
    if ( preemptiveStart == NULL ) {
        return;
    }
    global_bytesize swapleft = swap->getFreeSwap();
    bytes = min ( preemptiveBytes, bytes );
    bytes = min ( swapleft, bytes );
    swapleft = min ( preemptiveBytes, swapleft ); //This makes the hard limit clear.
    cyclicAtime *cur = preemptiveStart;
    global_bytesize bytesselected = 0;
    unsigned int chunks = 0;
    bool consecutive = true;
    while ( cur != active && bytesselected < bytes ) {
        if ( cur->chunk->size + bytesselected < swapleft && cur->chunk->status & MEM_ALLOCATED && cur->chunk->useCnt == 0 ) {
            bytesselected += cur->chunk->size;
            ++chunks;
            cur->chunk->preemptiveLoaded = false;
        } else {
            consecutive = false;
        }
        cur = cur->next;
    }
    if ( cur == preemptiveStart ) {
        return;
    }
    cyclicAtime *cur2 = preemptiveStart;
    managedMemoryChunk *chunklist[chunks];
    managedMemoryChunk **cursw = chunklist;
    bytesselected = 0;
    while ( cur2 != cur && bytesselected < bytes ) {
        if ( cur2->chunk->size + bytesselected < swapleft && cur2->chunk->status & MEM_ALLOCATED && cur2->chunk->useCnt == 0 ) {
            *cursw = cur2->chunk;
            ++cursw;
            bytesselected += cur2->chunk->size;
        }
        cur2 = cur2->next;
    }
    if ( swap->swapOut ( chunklist, chunks ) != bytesselected ) {
        Throw ( rambrain::memoryException ( "Could not swap out bytes even though swap claimed to be able to swap out this." ) );
    }
    preemptiveBytes -= bytesselected;
#ifdef SWAPSTATS
    ++n_swap_out;
    swap_out_scheduled_bytes += bytesselected;
#endif

    rambrain_pthread_mutex_lock ( &cyclicTopoLock );

    if ( !consecutive ) {
        //Take out all chunks that we have swapped out:
        cyclicAtime *from = preemptiveStart;
        preemptiveStart = preemptiveStart->prev;
        const memoryStatus filterMe[] = {MEM_SWAPOUT, MEM_SWAPPED, MEM_ROOT}; //Mem_root terminates list.
        chain area = {from, cur};
        struct chain separated = filterChain ( area, filterMe ); // After this action, from->prev->next  to cur2 only holds preemtive stuff.
        preemptiveStart = preemptiveStart->next;//Lets move again into area that only holds preemptives now.
        insertBefore ( preemptiveStart, separated );
        preemptiveStart = ( preemptiveStart == active ? NULL : preemptiveStart );

    } else {
        preemptiveStart = ( cur == active ? NULL : cur );

    }


    rambrain_pthread_mutex_unlock ( &cyclicTopoLock );

}


bool cyclicManagedMemory::swapIn ( managedMemoryChunk &chunk )
{
    VERBOSEPRINT ( "swapInEntry" );
#ifdef VERYVERBOSE
    if ( chunk.useCnt == 0 ) {
        printf ( "Unprotected!!!\n;" );
    }
    printf ( "Main Subject %lu\n", chunk.id );
#endif
    if ( chunk.status & MEM_ALLOCATED || chunk.status == MEM_SWAPIN ) {
        return true;
    }

    global_bytesize max_preemptive = ( swapInFrac - swapOutFrac ) * memory_max;
    double prob_random_preempt = pow ( swapInFrac - swapOutFrac, consecutivePreemptiveTransactions );

    if ( 0.01 > prob_random_preempt || pow ( swapInFrac - swapOutFrac, preemptiveSinceLast ) > .01 ) {
        global_bytesize preemptiveReduction = 2.* ( max_preemptive - preemptiveBytes ) + 1;
        decay ( preemptiveReduction );
    }
    consecutivePreemptiveTransactions = 0;

    // We use the old border to ensure that sth is not swapped in again that was just swapped out.
    cyclicAtime *oldBorder = counterActive;


    global_bytesize actual_obj_size = chunk.size;
    //We want to read in what the user requested plus fill up the preemptive area with opportune guesses

    bool preemptiveAutoon = ( ( double ) swap->getFreeSwap() ) / swap->getSwapSize() > preemptiveTurnoffFraction;

    if ( preemtiveSwapIn && preemptiveAutoon ) {
#ifdef VERYVERBOSE
        if ( preemptiveBytes > max_preemptive ) {
            warnmsg ( "Loaded more premptively than suggested!" );
        }
#endif

        global_bytesize targetReadinVol = actual_obj_size + ( swapInFrac - swapOutFrac ) * memory_max - preemptiveBytes;
#ifdef VERYVERBOSE
        printf ( "Preemptive swapin (premptiveBytes = %lu) (targetReadinVol = %lu)\n", preemptiveBytes, targetReadinVol );
#endif

        //Swap out if we want to read in more than what we thought:

        if ( targetReadinVol + memory_used > memory_max ) {
#ifdef VERYVERBOSE
            printf ( "We do not have space to get fully preemptive, lets try swap something out\n" );
#endif
            global_bytesize targetSwapoutVol = actual_obj_size + ( 1. - swapOutFrac ) * memory_max - preemptiveBytes;
            targetReadinVol = targetSwapoutVol;
            swapErrorCode err = swapOut ( targetReadinVol ); // A simple call to ensureEnoughSpace is not enough, we want to control what happens on error.
            if ( err != ERR_SUCCESS ) {
#ifdef VERYVERBOSE
                printf ( "We could not swap out enough, lets retry with the object only (which is the minimum)\n" );
#endif
                //We could not swap out enough, lets retry with the object only (which is the minimum)
                bool alreadyThere = ensureEnoughSpace ( actual_obj_size, &chunk );
                if ( alreadyThere ) {
                    waitForSwapin ( chunk, true );
                    VERBOSEPRINT ( "Is Already swapped in by so else" );
                    return true;
                }
                targetReadinVol = actual_obj_size;
            }
            if ( ensureEnoughSpace ( targetReadinVol, &chunk ) ) {
                waitForSwapin ( chunk, true );
                VERBOSEPRINT ( "Is Already swapped in by so else" );
                return true;
            }
        }
#ifdef VERYVERBOSE
        else {
            printf ( "We got enough space right away.\n" );
        }
#endif
        VERBOSEPRINT ( "swapInAfterSwap" );
        cyclicAtime *readEl = ( cyclicAtime * ) chunk.schedBuf;
        cyclicAtime *cur = readEl;
        cyclicAtime *endSwapin = readEl;
        global_bytesize selectedReadinVol = 0;
        global_bytesize preemtivelySelected = 0;
        unsigned int numberSelected = 0;
        rambrain_pthread_mutex_lock ( &cyclicTopoLock );

#ifdef VERYVERBOSE
        printf ( "Starting swapin selection" );
#endif

        max_preemptive -= preemptiveBytes; //Our limit for this transaction.

        //Why do we not have to check for chunk's status?
        // Because, as we should load in a swapped element, we're in the swapped section,
        // which only contains swapped elements until counterActive is reached.
        do {
#ifdef VERYVERBOSE
            printf ( "Chunk %lu has %lu bytes, this is %ld over the top\n", cur->chunk->id, cur->chunk->size, selectedReadinVol + cur->chunk->size + memory_used - memory_max );
#endif
            if ( selectedReadinVol + cur->chunk->size + memory_used <= memory_max && cur->chunk->status == MEM_SWAPPED && ( selectedReadinVol == 0 || ( preemtivelySelected + cur->chunk->size <= max_preemptive ) ) ) {

                cur->chunk->preemptiveLoaded = ( selectedReadinVol > 0 ? true : false );
                ++numberSelected;


                if ( selectedReadinVol > 0 ) {
                    preemtivelySelected += cur->chunk->size;
                }
                selectedReadinVol += cur->chunk->size;
                if ( selectedReadinVol >= targetReadinVol || ( selectedReadinVol > 0 && preemtivelySelected == max_preemptive ) ) {
                    break;
                }
#ifdef VERYVERBOSE
                printf ( "swapin %d\n", cur->chunk->id );
#endif
            }
            cur = cur->prev;
        } while ( cur != oldBorder );

        managedMemoryChunk *chunks[numberSelected];
        unsigned int n = 0;
        global_bytesize selectedReadinVol2 = 0;
        preemtivelySelected = 0;
        bool activeInList = false;

        do {
            if ( readEl == active ) {
                activeInList = true;
            }
            if ( selectedReadinVol2 + readEl->chunk->size + memory_used <= memory_max && readEl->chunk->status == MEM_SWAPPED && ( selectedReadinVol2 == 0 || ( preemtivelySelected + readEl->chunk->size <= max_preemptive ) ) ) {
                chunks[n++] = readEl->chunk;

                if ( selectedReadinVol2 > 0 ) {
                    preemtivelySelected += readEl->chunk->size;
                }
                selectedReadinVol2 += readEl->chunk->size;
                if ( selectedReadinVol2 >= targetReadinVol || ( selectedReadinVol2 > 0 && preemtivelySelected == max_preemptive ) ) {
                    break;
                }
            }
            readEl = readEl->prev;
        } while ( readEl != oldBorder );
        preemptiveSinceLast = numberSelected - 1;

        global_bytesize swappedInBytes = swap->swapIn ( chunks, numberSelected );
        if ( (  swappedInBytes != selectedReadinVol ) ) {
            //Check if we at least have swapped in enough:
            VERBOSEPRINT ( "exiting with non complete job" );
            if ( ! ( chunk.status & MEM_ALLOCATED || chunk.status == MEM_SWAPIN ) ) {
                return Throw ( memoryException ( "managedSwap failed to swap in :-(" ) );
            }

        }
#ifdef VERYVERBOSE
        printf ( "endSwapin is at %lu", chunks[numberSelected - 1]->id );
#endif


        VERBOSEPRINT ( "Before reordering" );
        preemptiveBytes += selectedReadinVol - actual_obj_size;

        if ( readEl == oldBorder ) { // Correct for boundary too long when hitting counterActive.
            readEl = readEl->next;
        }

        //one difficulty here is that active ptr marks our insertion point. However, it is not easy to track where active
        //actually is sitting in, be it the filtered section, the 'good' section or outside the filtered area. Normally,
        //it is just outside, however in rare circumstances, active is inside. We need to keep track of these cases without
        //producing much overhead. The following seems to work now and we hope it captures all circumstances.
        bool allElementsLoadedin = false;
        if ( activeInList ) { // Correct when active was also moved in action. This will exclude cyclic things downstairs, as we have an element out of here, endSwapin.
            if ( active == endSwapin ) {
                active = active->next;
            } else {
                allElementsLoadedin = true;
            }
        }

        cyclicAtime *after = endSwapin->next;
        if ( after == readEl ) {
            after = NULL;    //mark if were cyclic.
        }

        chain toFilter = {readEl, endSwapin};

        const memoryStatus justSwappedin[] = {MEM_SWAPIN, MEM_ALLOCATED, MEM_ALLOCATED_INUSE_READ, MEM_ALLOCATED_INUSE_WRITE, MEM_ROOT};
        chain filtered = filterChain ( toFilter, justSwappedin );
        if ( !preemptiveStart ) {
            preemptiveStart = filtered.from;
            if ( preemptiveStart && !preemptiveStart->chunk->preemptiveLoaded ) { ///@todo in rare cases, these two lines are necessary as the readEl is also filtered. We should find out if we can leave this out somehow more elegant.
                preemptiveStart = NULL;
            }
        }
        //Let us now reinsert:
        if ( after ) { // We are not cyclic
            if ( allElementsLoadedin ) {
                active = toFilter.from;
            }
            if ( active ) {
                insertBefore ( active, filtered );
            } else {
                active = after;
                insertBefore ( after, filtered );
            }
        } else { // We are cyclic
            counterActive = readEl;
            if ( toFilter.from ) {
                insertBefore ( toFilter.from, filtered );
            } else {
                MUTUAL_CONNECT ( filtered.to, filtered.from )
            }

        }
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        touch ( chunk );
        if ( counterActive->chunk->status == MEM_SWAPPED ) {
            counterActive = active;
        }

#ifdef SWAPSTATS
        swap_in_scheduled_bytes += selectedReadinVol;
        n_swap_in += 1;
#endif
        VERBOSEPRINT ( "swapInBeforeReturn" );
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
            rambrain_pthread_mutex_lock ( &cyclicTopoLock );
            if ( counterActive->chunk->status == MEM_SWAPPED ) {
                counterActive = active;
            }

#ifdef SWAPSTATS
            swap_in_scheduled_bytes += chunk.size;
            n_swap_in += 1;
#endif
            rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
            return true;
        } else {
            //Unlock mutex under which we were called, as we'll be throwing...
            rambrain_pthread_mutex_unlock ( &stateChangeMutex );
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
    rambrain_pthread_mutex_lock ( &stateChangeMutex );
    rambrain_pthread_mutex_lock ( &cyclicTopoLock );
#ifdef PARENTAL_CONTROL
    unsigned int no_reg = memChunks.size() - 1;
#else
    unsigned int no_reg = memChunks.size();
#endif
    unsigned int encountered = 0;
    cyclicAtime *cur = active;
    cyclicAtime *oldcur;

    global_bytesize usedBytes = 0;
    global_bytesize tobef = 0;

    global_bytesize swappedBytes = 0;

    if ( !cur ) {
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        rambrain_pthread_mutex_unlock ( &stateChangeMutex );
        if ( no_reg == 0 && counterActive == NULL ) {
            return true;
        } else {
            errmsg ( "No active element found" );
            return false;
        }
    }

    bool inActiveOnlySection = ( active->chunk->status == MEM_SWAPPED || active->chunk->status == MEM_SWAPOUT ? false : true );
    bool inSwapsection = true;
    bool memerror = false;
    bool hasPreemptives = false;
    do {
        ++encountered;
        oldcur = cur;
        cur = cur->next;
        if ( cur->chunk->preemptiveLoaded ) {
            hasPreemptives = true;
        }
        if ( cur->chunk->status & MEM_ALLOCATED || cur->chunk->status == MEM_SWAPIN ) {
            usedBytes += cur->chunk->size;
        } else if ( cur->chunk->status == MEM_SWAPOUT ) {
            tobef += cur->chunk->size;
            usedBytes += cur->chunk->size;
        }
        if ( cur->chunk->status == MEM_SWAPPED || cur->chunk->status == rambrain::MEM_SWAPIN || cur->chunk->status == MEM_SWAPOUT ) {
            swappedBytes += cur->chunk->size;
        }

        if ( oldcur != cur->prev ) {
            errmsgf ( "Mutual connecion failure at chunks %lu and %lu", oldcur->chunk->id, cur->chunk->id );
            memerror = true;
        }

        if ( inActiveOnlySection ) {
            if ( oldcur->chunk->status == MEM_SWAPPED || oldcur->chunk->status == MEM_SWAPOUT ) {
                errmsg ( "Swapped elements in active section!" );
                memerror = true;
            }
        } else {
            if ( oldcur->chunk->status == MEM_SWAPPED && !inSwapsection ) {
                errmsg ( "Isolated swapped element block not tracked by counterActive found!" );
                memerror = true;
            }
            if ( oldcur->chunk->status != MEM_SWAPPED && oldcur->chunk->status != MEM_SWAPOUT ) {
                inSwapsection = false;
            }
        }

        if ( oldcur == counterActive ) {
            inActiveOnlySection = false;
        }

    } while ( cur != active );

    if ( usedBytes != memory_used ) {
        errmsgf ( "Used bytes are not counted correctly, claimed %lu but found %lu", memory_used, usedBytes );
        memerror = true;
    }
    if ( tobef != memory_tobefreed ) {
        errmsgf ( "To-Be-Freed bytes are not counted correctly, claimed %lu but found %lu", memory_used, usedBytes );
        memerror = true;
    }
    if ( swappedBytes != swap->getUsedSwap() ) {
        errmsgf ( "Swapped bytes are not counted correctly, claimed %lu but found %lu", memory_used, usedBytes );
        memerror = true;
    }


    unsigned int illicit_count = 0;
    if ( preemptiveStart ) {
        cur = preemptiveStart;
        hasPreemptives = true;
        while ( cur != active ) {
            if ( cur->chunk->preemptiveLoaded == false ) {
                if ( illicit_count < 10 ) {
                    errmsgf ( "Chunk %ld is a illicit non-preemptive.", cur->chunk->id );
                } else {
                    if ( illicit_count == 10 ) {
                        errmsg ( "will not report further illicits" );
                    }
                }
                ++illicit_count;
                hasPreemptives = false;
            }
            cur = cur->next;
        }
        if ( !hasPreemptives ) {
            errmsg ( "We encountered non-preemptive elements in section marked as preemptive" );
            memerror = true;
        }
        hasPreemptives = false;
        illicit_count = 0;
        while ( cur != preemptiveStart ) {
            if ( cur->chunk->preemptiveLoaded == true ) {
                if ( illicit_count < 10 ) {
                    errmsgf ( "Chunk %ld is a illicit preemptive.", cur->chunk->id );
                } else {
                    if ( illicit_count == 10 ) {
                        errmsg ( "will not report further illicits" );
                    }
                }
                ++illicit_count;
                hasPreemptives = true;
            }
            cur = cur->next;
        }
        if ( hasPreemptives ) {
            errmsg ( "We encountered preemptive elements in section marked as non-preemptive" );
            memerror = true;
        }

    } else {
        if ( hasPreemptives ) {
            errmsg ( "We have preemptives but no preemptiveStart is assigned" );
            memerror = true;
        }

    }

    rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
    rambrain_pthread_mutex_unlock ( &stateChangeMutex );
    if ( encountered != no_reg ) {
        errmsgf ( "Not all elements accessed. %d expected,  %d encountered", no_reg, encountered );
        memerror = true;
    }

    if ( memerror ) {
        printCycle();
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
    if ( preemptiveStart ) {
        printf ( "%lu (%s)<-preemptiveStart\n", preemptiveStart->chunk->id, ( preemptiveStart->chunk->preemptiveLoaded ? "p" : " " ) );
    }
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
            printf ( "%lu (%s) %u%s <-counterActive\t", atime->chunk->id, ( atime->chunk->preemptiveLoaded ? "p" : " " ), atime->chunk->useCnt, status );
        } else {
            printf ( "%lu (%s) %u%s \t", atime->chunk->id, ( atime->chunk->preemptiveLoaded ? "p" : " " ), atime->chunk->useCnt, status );
        }
        atime = atime->next;
    } while ( atime != active );
    printf ( "In Preemptive: %lu ( of %lf )\n", preemptiveBytes , memory_max * ( swapInFrac - swapOutFrac ) );
    printf ( "\n" );
    printf ( "\n" );
}

void cyclicManagedMemory::printChain ( const char *name, const cyclicManagedMemory::chain mchain )
{
    cyclicAtime *cur = mchain.from;
    if ( !cur ) {
        printf ( "Chain named '%s' is empty\n", name );
        return;
    }

    printf ( "Chain named '%s'", name );
    while ( true ) {
        printf ( "->%lu\t", cur->chunk->id );
        if ( cur == mchain.to ) {
            break;
        }
        cur = cur->next;
    }
    printf ( "\n" );
}


cyclicManagedMemory::swapErrorCode cyclicManagedMemory::swapOut ( rambrain::global_bytesize min_size )
{

    rambrain_pthread_mutex_lock ( &cyclicTopoLock );
    if ( counterActive == 0 ) {
        rambrain_pthread_mutex_unlock ( &stateChangeMutex );
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        Throw ( memoryException ( "I can't swap out anything if there's nothing to swap out." ) );
    }
    VERBOSEPRINT ( "swapOutEntry" );
    if ( min_size > memory_max ) {
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        return ERR_MORETHANTOTALRAM;
    }
    global_bytesize swap_free = swap->getFreeSwap();
    if ( swap_free < min_size ) {
        swap->cleanupCachedElements ( min_size - swap_free );
        swap_free = swap->getFreeSwap();
        if ( swap_free < min_size ) {
            if ( !swap->extendSwapByPolicy ( min_size - swap_free ) ) {
                rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
                return ERR_SWAPFULL;
            }
            swap_free = swap->getFreeSwap();
        }

    }

    global_bytesize mem_alloc_max = memory_max * swapOutFrac; //<- This is target size
    global_bytesize mem_swap_min = memory_used > mem_alloc_max ? memory_used - mem_alloc_max : 0;
    global_bytesize mem_swap = mem_swap_min < min_size ? min_size : mem_swap_min; // swap at least what you have to

    mem_swap = mem_swap > swap_free ? min_size : mem_swap;//But do not swap more than swap can take (Or try with only min_size)

    cyclicAtime *fromPos = counterActive;
    cyclicAtime *countPos = counterActive;
    global_bytesize unload_size = 0;
    global_bytesize unload_size2 = 0;
    unsigned int passed = 0, unload = 0;
#ifdef PARENTAL_CONTROL
    unsigned int allelements = memChunks.size() - 1 ;
#else
    unsigned int allelements = memChunks.size();
#endif


    //First round: Calculate number of objects to swap out.
    while ( unload_size < mem_swap ) {
        ++passed;
        if ( countPos->chunk->status == MEM_ALLOCATED && ( unload_size + countPos->chunk->size <= swap_free )  && ( countPos->chunk->useCnt == 0 ) ) {
            if ( countPos->chunk->size + unload_size <= swap_free ) {
                unload_size += countPos->chunk->size;
                ++unload;
#ifdef VERYVERBOSE
                printf ( "U(%d)\t", countPos->chunk->id );
#endif
            }
        }

        if ( passed == allelements ) {
#ifdef VERYVERBOSE
            printf ( "emergency, once round!\n" );
#endif
            break;
        }

        countPos = countPos->prev;
    }
    if ( unload_size == 0 ) {
#ifdef VERYVERBOSE
        printf ( "no candidates found\n" );
#endif
        rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
        return ERR_NOTENOUGHCANDIDATES;
    }

    managedMemoryChunk *unloadlist[unload];
    managedMemoryChunk **unloadElem = unloadlist;
#ifdef VERYVERBOSE
    printf ( "active = %d\n", active->chunk->id );
#endif
    passed = 0;
    bool resetPreemptiveStart = false;
    while ( unload_size2 < unload_size && passed != allelements ) {
        ++passed;
        if ( fromPos->chunk->status == MEM_ALLOCATED && ( unload_size2 + fromPos->chunk->size <= swap_free ) && ( fromPos->chunk->useCnt == 0 ) ) {
            if ( fromPos->chunk->size + unload_size2 <= swap_free ) {
                unload_size2 += fromPos->chunk->size;
                *unloadElem = fromPos->chunk;
                ++unloadElem;
#ifdef VERYVERBOSE
                printf ( "swapout %d\n", fromPos->chunk->id );
#endif
                if ( fromPos->chunk->preemptiveLoaded ) { //We had this chunk preemptive, but now have to swap out.
                    //This is a bit evil, as we will reload preemptive bytes when we've swapped them out.
                    ///@todo investigate if subtracting swapped out preemptive bytes is affecting performance ( too much preemptive action possible ). Naively testing, this is not the case.
                    fromPos->chunk->preemptiveLoaded = false;
                    preemptiveBytes -= fromPos->chunk->size;
                }
                if ( preemptiveStart && ( fromPos->chunk == preemptiveStart->chunk ) ) {
                    resetPreemptiveStart = true;
                }
            }
        }
        fromPos = fromPos->prev;
    }
    global_bytesize real_unloaded = swap->swapOut ( unloadlist, unload );
    bool swapSuccess = ( real_unloaded >= mem_swap ) ; // Do not compare with unload size (false positives!)
    if ( !swapSuccess ) {
        if ( real_unloaded == 0 ) {
            rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
#ifdef VERYVERBOSE
            printf ( "Swap out did not work (%lu vs %lu )\n", min_size, real_unloaded );
#endif
            return ERR_NOTENOUGHCANDIDATES;
        }
    }
#ifdef VERYVERBOSE
    printf ( "active = %d\n", active->chunk->id );
#endif
    fromPos = fromPos->next;
    chain possiblyContaminated = {fromPos, counterActive};
    cyclicAtime *after = counterActive->next;
    if ( after == fromPos ) { // We're cyclic
        after = NULL;
    }
    memoryStatus notAllowed[] = {MEM_SWAPOUT, MEM_SWAPPED, MEM_ROOT};
    counterActive = possiblyContaminated.from->prev;
    chain filtered = filterChain ( possiblyContaminated, notAllowed );
    //Possibilities:
    if ( !possiblyContaminated.from ) { // filtered out all selected
        if ( after ) { //However, there are still other elements around.
            insertBefore ( after, filtered );


        } else { //We're left with what we have
            MUTUAL_CONNECT ( filtered.to, filtered.from );
            counterActive = active;
        }

    } else {
        //We will have filtered out some of them.
        if ( !after ) {
            after = possiblyContaminated.to->next;
        }
        insertBefore ( after, filtered );
        counterActive = filtered.from->prev;
    }
    if ( ! ( active->chunk->status & MEM_ALLOCATED || active->chunk->status == MEM_SWAPIN ) ) { // We may have swapped out the first allocated element
        active = counterActive;
#ifdef VERYVERBOSE
        printf ( "had to move active", fromPos->chunk->id );
#endif
    }
    if ( resetPreemptiveStart ) { //Rare case!
        cyclicAtime *cur = active;

        while ( cur->prev->chunk->preemptiveLoaded ) {
            cur = cur->prev;
        }
        preemptiveStart = ( cur == active ? NULL : cur );
    }
    rambrain_pthread_mutex_unlock ( &cyclicTopoLock );
    VERBOSEPRINT ( "swapOutReturn" );
    if ( swapSuccess ) {
#ifdef VERYVERBOSE
        printf ( "Swap out succeeded (%lu vs %lu )\n", min_size, real_unloaded );
#endif
#ifdef SWAPSTATS
        swap_out_scheduled_bytes += unload_size;
        n_swap_out += 1;
#endif
        return ERR_SUCCESS;
    } else {
#ifdef VERYVERBOSE
        printf ( "Swap out (%lu vs %lu )\n", min_size, real_unloaded );
#endif
        if ( real_unloaded >= min_size ) {
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


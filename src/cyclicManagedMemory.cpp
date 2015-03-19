#include "cyclicManagedMemory.h"
#include "common.h"
#include "exceptions.h"

// #define VERYVERBOSE

#ifdef VERYVERBOSE
#define VERBOSEPRINT(x) printf("\n<--%s\n",x);printCycle();printf("\n%s---->\n",x);
#else
#define VERBOSEPRINT(x) ;
#endif



cyclicManagedMemory::cyclicManagedMemory ( managedSwap *swap, unsigned int size ) : managedMemory ( swap, size )
{

}

void cyclicManagedMemory::schedulerRegister ( managedMemoryChunk &chunk )
{

    cyclicAtime *neu = new cyclicAtime;

    //Couple chunk to atime and vice versa:
    neu->chunk = &chunk;
    chunk.schedBuf = ( void * ) neu;

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
}



void cyclicManagedMemory::schedulerDelete ( managedMemoryChunk &chunk )
{
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;

    //Hook out element:
    if ( element->next == element ) {
        active = NULL;
        counterActive = NULL;
        delete element;
        return;

    }

    if ( element->next == element->prev ) {
        cyclicAtime *remaining = element->next;
        remaining->next = remaining->prev = remaining;
    } else {
        element->next->prev = element->prev;
        element->prev->next = element->next;
    }
    if ( chunk.status == MEM_SWAPPED ) {
        memory_swapped -= chunk.size;
    } else if ( counterActive->chunk->atime > chunk.atime ) { //preemptively loaded
        preemptiveBytes -= chunk.size;
    }
    if ( active == element ) {
        active = element->next;
    }

    if ( counterActive == element ) {
        counterActive = element->prev;
    }

    delete element;
}
//Touch happens automatically after use, create, swapIn
bool cyclicManagedMemory::touch ( managedMemoryChunk &chunk )
{
    if ( counterActive->chunk->atime > chunk.atime ) { //This chunk was preemptively loaded
        preemptiveBytes -= chunk.size;
    }


    //Put this object to begin of loop:
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;



    if ( active == element ) {
        return true;
    }

    if ( counterActive == element ) {
        counterActive = counterActive->prev;
    }

    chunk.atime = atime++; //increase atime only if we accessed a different element in between

    if ( active->prev == element ) {
        active = element;
        return true;
    };



    if ( active->next == element ) {
        cyclicAtime *before = active->prev;
        cyclicAtime *after = element->next;
        MUTUAL_CONNECT ( before, element );
        MUTUAL_CONNECT ( element, active );
        MUTUAL_CONNECT ( active, after );
        active = element;
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
    unsigned int claimed_use = swap->getUsedSwap();
    fprintf ( stderr, "%u\t%u=%u\t%u\n", memory_used, claimed_use, memory_swapped, preemptiveBytes );
}


bool cyclicManagedMemory::swapIn ( managedMemoryChunk &chunk )
{
    VERBOSEPRINT ( "swapInEntry" );
    // We use the old border to ensure that sth is not swapped in again that was just swapped out.
    cyclicAtime *oldBorder = counterActive;


    unsigned int actual_obj_size = chunk.size;
    //We want to read in what the user requested plus fill up the preemptive area with opportune guesses



    if ( preemtiveSwapIn ) {
        unsigned int targetReadinVol = actual_obj_size + ( swapInFrac - swapOutFrac ) * memory_max - preemptiveBytes;
        //fprintf(stderr,"%u %u %u %u %u\n",memory_used,preemptiveBytes,actual_obj_size,targetReadinVol,0);
        //Swap out if we want to read in more than what we thought:
        if ( targetReadinVol + memory_used > memory_max ) {
            unsigned int targetSwapoutVol = actual_obj_size + ( 1. - swapOutFrac ) * memory_max - preemptiveBytes;

            if ( !swapOut ( targetSwapoutVol ) ) {
                if ( memory_used + actual_obj_size > memory_max ) {
                    if ( swapOut ( memory_used + actual_obj_size - memory_max ) ) {
                        targetReadinVol = actual_obj_size;
                    } else {
                        return Throw ( memoryException ( "Out of Memory." ) );
                    }
                } else {
                    targetReadinVol = actual_obj_size;
                }
            }


        }
        VERBOSEPRINT ( "swapInAfterSwap" );
        cyclicAtime *readEl = ( cyclicAtime * ) chunk.schedBuf;
        cyclicAtime *cur = readEl;
        cyclicAtime *endSwapin = readEl;
        unsigned int selectedReadinVol = 0;
        unsigned int numberSelected = 0;

        do {
            if ( selectedReadinVol + cur->chunk->size > targetReadinVol ) {
                break;
            }
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
        if ( ( swap->swapIn ( chunks, numberSelected ) != n ) ) {
            printf ( "oink!\n" );
            return Throw ( memoryException ( "managedSwap failed to swap in :-(" ) );

        } else {
            cyclicAtime *beginSwapin = readEl->next;
            cyclicAtime *oldafter = endSwapin->next;
            if ( endSwapin != active ) { //swapped in element is already 'active' when all others have been swapped.
                if ( oldafter != active && beginSwapin != active ) {
                    //TODO: Implement this for <3 elements
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



            memory_used += selectedReadinVol;
            memory_swapped -= selectedReadinVol;
            preemptiveBytes += selectedReadinVol - actual_obj_size;
            chunk.atime = atime++;

#ifdef SWAPSTATS
            swap_in_bytes += selectedReadinVol;
            n_swap_in += 1;
#endif
            VERBOSEPRINT ( "swapInBeforeReturn" );
            return true;
        }
    } else {
        ensureEnoughSpaceFor ( actual_obj_size );
        if ( swap->swapIn ( &chunk ) ) {
            memory_used += chunk.size;
            memory_swapped -= chunk.size;
#ifdef SWAPSTATS
            swap_in_bytes += chunk.size;
            n_swap_in += 1;
#endif
            if ( chunk.schedBuf == counterActive ) {
                counterActive = counterActive->prev;
            }
            return true;
        } else {
            return Throw ( memoryException ( "Could not swap in an element." ) );
        };
    }

}


//Idea: swap out more than required, as the free space may be filled with premptive swap-ins

bool cyclicManagedMemory::checkCycle()
{
    unsigned int no_reg = memChunks.size() - 1;
    unsigned int encountered = 0;
    cyclicAtime *cur = active;
    cyclicAtime *oldcur;

    if ( !cur ) {
        if ( no_reg == 0 && counterActive == NULL ) {
            return true;
        } else {
            return false;
        }
    }

    bool inActiveOnlySection = ( active->chunk->status == MEM_SWAPPED ? false : true );
    bool inSwapsection = true;
    do {
        ++encountered;
        oldcur = cur;
        cur = cur->next;
        if ( oldcur != cur->prev ) {
            errmsg ( "Mutual connecion failure" );
            return false;
        }

        if ( inActiveOnlySection ) {
            if ( oldcur->chunk->status == MEM_SWAPPED ) {
                errmsg ( "Swapped elements in active section!" );
                return false;
            }
        } else {
            if ( oldcur->chunk->status == MEM_SWAPPED && !inSwapsection ) {
                errmsg ( "Isolated swapped element block not tracked by counterActive found!" );
                return false;
            }
            if ( oldcur->chunk->status != MEM_SWAPPED ) {
                inSwapsection = false;
            }
        }

        if ( oldcur == counterActive ) {
            inActiveOnlySection = false;
        }

    } while ( cur != active );

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
    printf ( "%d (%d)<-counterActive\n", counterActive->chunk->id, counterActive->chunk->atime );
    printf ( "%d => %d => %d\n", counterActive->prev->chunk->id, counterActive->chunk->id, counterActive->next->chunk->id );
    printf ( "%d (%d)<-active\n", active->chunk->id, active->chunk->atime );
    printf ( "%d => %d => %d\n", active->prev->chunk->id, active->chunk->id, active->next->chunk->id );
    printf ( "\n" );
    do {
        char  status[2];
        status[1] = 0x00;
        switch ( atime->chunk->status ) {
        case MEM_ALLOCATED:
            status[0] = 'A';
            break;
        case MEM_SWAPPED:
            status[0] = 'S';
            break;
        case MEM_ALLOCATED_INUSE:
            status[0] = 'U';
            break;
        case MEM_ROOT:
            status[0] = 'R';
            break;
        }
        if ( atime == counterActive ) {
            printf ( "%d (%d) %s <-counterActive\n", atime->chunk->id, atime->chunk->atime, status );
        } else {
            printf ( "%d (%d) %s \n", atime->chunk->id, atime->chunk->atime, status );
        }
        atime = atime->next;
    } while ( atime != active );
    printf ( "\n" );
    printf ( "\n" );
}



bool cyclicManagedMemory::swapOut ( unsigned int min_size )
{
    VERBOSEPRINT ( "swapOutEntry" );
    if ( min_size > memory_max ) {
        return false;
    }
    unsigned int mem_alloc_max = memory_max * swapOutFrac; //<- This is target size
    unsigned int mem_swap_min = memory_used > mem_alloc_max ? memory_used - mem_alloc_max : 0;
    unsigned int mem_swap = mem_swap_min < min_size ? min_size : mem_swap_min;

    cyclicAtime *fromPos = counterActive;
    cyclicAtime *countPos = counterActive;
    unsigned int unload_size = 0, unload = 0;
    unsigned int passed = 0;
    while ( unload_size < mem_swap ) {
        ++passed;
        if ( countPos->chunk->status == MEM_ALLOCATED ) {
            unload_size += countPos->chunk->size;
            ++unload;
        }
        countPos = countPos->prev;
        if ( fromPos == countPos ) {
            break;
        }

    }
    if ( fromPos == countPos && unload_size < mem_swap ) { //We've been round one time and could not make it.
        return false;
    }

    managedMemoryChunk *unloadlist[unload];
    managedMemoryChunk **unloadElem = unloadlist;

    do {
        if ( fromPos->chunk->status == MEM_ALLOCATED ) {
            *unloadElem = fromPos->chunk;
            ++unloadElem;
            if ( fromPos == active ) {
                active = fromPos->prev;
            }
        }
        fromPos = fromPos->prev;
    } while ( fromPos != countPos );
    unsigned int real_unloaded = swap->swapOut ( unloadlist, unload );

    bool swapSuccess = ( real_unloaded == unload ) ;
    if ( !swapSuccess ) {
        return Throw ( memoryException ( "Could not swap out all elements. Swap full?" ) );

    }
    fromPos = counterActive;
    cyclicAtime *moveEnd, *cleanFrom;
    moveEnd = NULL;
    cleanFrom = counterActive;
    bool inSwappedSection = true;

    //TODO: Implement this for less than 3 elements!

    while ( fromPos != countPos ) {
        if ( inSwappedSection ) {
            if ( fromPos->chunk->status != MEM_SWAPPED ) {
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
            if ( fromPos->chunk->status == MEM_SWAPPED ) {
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
    VERBOSEPRINT ( "swapOutReturn" );
    if ( swapSuccess ) {

        memory_swapped += unload_size;
        memory_used -= unload_size;
#ifdef SWAPSTATS
        swap_out_bytes += unload_size;
        n_swap_out += 1;
#endif
        return true;
    }

    else {
        return false;
    }
}










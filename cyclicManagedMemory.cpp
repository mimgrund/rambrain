#include "cyclicManagedMemory.h"
#include "common.h"

cyclicManagedMemory::cyclicManagedMemory (managedSwap *swap, unsigned int size) : managedMemory ( swap,size )
{

}

void cyclicManagedMemory::schedulerRegister ( managedMemoryChunk& chunk )
{
    cyclicAtime *neu = new cyclicAtime;

    //Couple chunk to atime and vice versa:
    neu->chunk = &chunk;
    chunk.schedBuf = ( void * ) neu;

    if ( active == NULL ) { // We're inserting first one
        neu->prev = neu->next = neu;
        counterActive = neu;
        active = neu;
    } else { //We're inserting somewhen.
        cyclicAtime *before = active->prev;
        cyclicAtime *after = active;
        before->next = neu;
        after->prev = neu;
        neu->next = after;
        neu->prev = before;
        active = neu;
    }
}



void cyclicManagedMemory::schedulerDelete ( managedMemoryChunk& chunk )
{
    cyclicAtime *element = ( cyclicAtime * ) chunk.schedBuf;

    //Hook out element:
    if ( element->next==element->prev ) {
        chunk.schedBuf = NULL;

    } else {
        element->next->prev = element->prev;
        element->prev->next = element->next;
    }
    if ( chunk.status==MEM_SWAPPED ) {
        memory_swapped -= chunk.size;
    }
    if(active==element)
        active=element->next;

    if(counterActive==element)
        counterActive=element->prev;

    delete element;
}
//Touch happens automatically after use, create, swapIn
bool cyclicManagedMemory::touch ( managedMemoryChunk& chunk )
{
    chunk.atime = atime++;

    //Put this object to begin of loop:
    cyclicAtime *element = (cyclicAtime *)chunk.schedBuf;
    if(active==element||element->next==element->prev)
        return true;
    if(counterActive==element);
    counterActive=counterActive->prev;
    //Take out of loop
    element->next->prev = element->prev;
    element->prev->next = element->next;
    //Insert at beginning
    active->prev->next = element;
    active->next->prev = element;
    element->next = active->next;
    element->prev = active->prev;
    active = element;

    return true;
}


bool cyclicManagedMemory::swapIn ( managedMemoryChunk& chunk )
{
    if(swap->swapIn(&chunk)) {
        memory_used+= chunk.size;
        memory_swapped-= chunk.size;
        return true;
    } else {
        return false;
    };

}


//Idea: swap out more than required, as the free space may be filled with premptive swap-ins


bool cyclicManagedMemory::swapOut ( unsigned int min_size )
{
    if (min_size>memory_max)
        return false;
    unsigned int mem_alloc_max = memory_max*swapOutFrac; //<- This is target size
    unsigned int mem_swap_min = memory_used>mem_alloc_max?memory_used-mem_alloc_max:0;
    unsigned int mem_swap = mem_swap_min<min_size?min_size:mem_swap_min;

    cyclicAtime *fromPos = counterActive;
    unsigned int unload_size=0,unload=0;

    while(unload_size<mem_swap) {
        if(counterActive->chunk->status==MEM_ALLOCATED)
            unload_size+=counterActive->chunk->size;
        counterActive=counterActive->prev;
        if(fromPos==counterActive)
            break;
        ++unload;
    }
    if(fromPos==counterActive) { //We've been round one time and could not make it.
        errmsg("Cannot swap as too much memory is used to satisfy swap requirement.");
        return false;
    }

    managedMemoryChunk *unloadlist[unload];
    managedMemoryChunk **unloadElem = unloadlist;

    while(fromPos!=counterActive) {
        if(fromPos->chunk->status==MEM_ALLOCATED) {
            *unloadElem = fromPos->chunk;
            ++unloadElem;
        }
        fromPos = fromPos->prev;

    }

    if(swap->swapOut(unloadlist,unload)!=unload)
        return false;
    else {
        memory_swapped+=unload_size;
        memory_used-=unload_size;

        return true;
    }
}


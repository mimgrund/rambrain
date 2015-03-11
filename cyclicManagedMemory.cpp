#include "cyclicManagedMemory.h"

cyclicManagedMemory::cyclicManagedMemory (managedSwap *swap, unsigned int size) : managedMemory ( swap,size )
{

}

void cyclicManagedMemory::schedulerRegister ( managedMemoryChunk& chunk )
{
    cyclicAtime *neu = new cyclicAtime;

    //Couple chunk to atime and vice versa:
    neu->chunk = &chunk;
    chunk.schedBuf = ( void * ) neu;

    if ( active == 0 ) { // We're inserting first one
        neu->prev = neu->next = neu;
        counterActive = neu;
    } else { //We're inserting somewhen.
        neu->next = active;
        neu->prev = active->prev;
        active->prev->next = neu;
        active->next->prev = neu;
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
        element->prev = element->prev->prev;
    }
    if ( chunk.status==MEM_SWAPPED ) {
        memory_swapped -= chunk.size;
    }

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
    element->prev = active->prev;
    active->prev = element;
    element->next = active;
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
        unload_size+=counterActive->chunk->size;
        counterActive=counterActive->prev;
        ++unload;
    }
    managedMemoryChunk *unloadlist[unload];
    managedMemoryChunk **unloadElem = unloadlist;

    while(fromPos!=counterActive) {
        *unloadElem = fromPos->chunk;
        fromPos = fromPos->next;
        ++unloadElem;
    }

    if(swap->swapOut(unloadlist,unload)!=unload)
        return false;
    else
        return true;
}


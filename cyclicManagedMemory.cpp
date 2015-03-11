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
        counterAtime = chunk.atime;
        counterActive = neu;
    } else { //We're inserting somewhen.
        neu->next = active->next;
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

bool cyclicManagedMemory::touch ( managedMemoryChunk& chunk )
{
    chunk.atime = atime++;

    //TODO: Rest of logic.
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

bool cyclicManagedMemory::swapOut ( unsigned int min_size )
{
    return false;
}

#ifndef CYCLICMANAGEDMEMORY_H
#define CYCLICMANAGEDMEMORY_H

#include "managedMemory.h"

struct cyclicAtime {
    managedMemoryChunk *chunk;
    cyclicAtime *next;
    cyclicAtime *prev;
    unsigned short pageFile;
    unsigned int pageOffset;
};


class cyclicManagedMemory : public managedMemory
{
public:
    cyclicManagedMemory ( managedSwap* swap, unsigned int size );
private:
    virtual bool swapIn ( managedMemoryChunk& chunk );
    virtual bool swapOut ( unsigned int min_size );
    virtual bool touch ( managedMemoryChunk& chunk );
    virtual void schedulerRegister ( managedMemoryChunk &chunk );
    virtual void schedulerDelete ( managedMemoryChunk &chunk );

    //loop pointers:
    cyclicAtime *active=NULL;
    cyclicAtime *counterActive=NULL;
    memoryAtime counterAtime;
    unsigned int diff=0;

    double swapOutFrac = .8;
    double swapInFrac = .9;
};


/*Philosophy:
 *
 *
 *
 *
 *
 *
 *
 * */




#endif

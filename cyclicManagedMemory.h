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
    void printCycle();
    bool checkCycle();
private:
    virtual bool swapIn ( managedMemoryChunk& chunk );
    virtual bool swapOut ( unsigned int min_size );
    virtual bool touch ( managedMemoryChunk& chunk );
    virtual void schedulerRegister ( managedMemoryChunk &chunk );
    virtual void schedulerDelete ( managedMemoryChunk &chunk );

    //loop pointers:
    cyclicAtime *active=NULL;
    cyclicAtime *counterActive=NULL;
    unsigned int diff=0;

    float swapOutFrac = .8;
    float swapInFrac = .9;
};

#define MUTUAL_CONNECT(A,B) A->next = B; B->prev = A;


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


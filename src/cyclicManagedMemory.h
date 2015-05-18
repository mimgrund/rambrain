#ifndef CYCLICMANAGEDMEMORY_H
#define CYCLICMANAGEDMEMORY_H

#include "managedMemory.h"

namespace membrain
{

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
    cyclicManagedMemory ( managedSwap *swap, unsigned int size );
    virtual ~cyclicManagedMemory() {}

    void printCycle();
    void printMemUsage();
    bool checkCycle();
    //Returns old value:
    bool setPreemptiveLoading ( bool preemptive );


private:
    virtual bool swapIn ( managedMemoryChunk &chunk );
    virtual bool swapOut ( unsigned int min_size );
    virtual bool touch ( managedMemoryChunk &chunk );
    virtual void schedulerRegister ( managedMemoryChunk &chunk );
    virtual void schedulerDelete ( managedMemoryChunk &chunk );

    //loop pointers:
    cyclicAtime *active = NULL;
    cyclicAtime *counterActive = NULL;
    unsigned int diff = 0;

    float swapOutFrac = .8;
    float swapInFrac = .9;

    bool preemtiveSwapIn = true; ///\todo Change strategy dynamically.
    unsigned int preemptiveBytes = 0;
    global_bytesize preemptiveTurnoffFraction = .01;

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

}

#endif

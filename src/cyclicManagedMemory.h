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
    cyclicManagedMemory ( membrain::managedSwap *swap, membrain::global_bytesize size );
    virtual ~cyclicManagedMemory() {}

    void printCycle();
    void printMemUsage();
    bool checkCycle();
    ///\note not thread-safe - does not make sense to call it from different threads anyway
    ///\note returns previous value
    bool setPreemptiveLoading ( bool preemptive );


private:
    ///\note protect call to swapIn by topologicalMutex
    virtual bool swapIn ( managedMemoryChunk &chunk );
    ///\note protect call to swapOut by topologicalMutex
    virtual bool swapOut ( global_bytesize min_size );
    virtual bool touch ( managedMemoryChunk &chunk );
    virtual void schedulerRegister ( managedMemoryChunk &chunk );
    virtual void schedulerDelete ( managedMemoryChunk &chunk );

    //loop pointers:
    cyclicAtime *active = NULL;
    cyclicAtime *counterActive = NULL;

    float swapOutFrac = .8;
    float swapInFrac = .9;

    bool preemtiveSwapIn = true; ///\todo Change strategy dynamically.
    global_bytesize preemptiveBytes = 0;

    static pthread_mutex_t cyclicTopoLock;

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

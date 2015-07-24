#ifndef CYCLICMANAGEDMEMORY_H
#define CYCLICMANAGEDMEMORY_H

#include "managedMemory.h"

namespace membrain
{
///@brief structure created by scheduler to track access times of memoryChunks
struct cyclicAtime {
    managedMemoryChunk *chunk;///The chunk
    cyclicAtime *next; ///Next chunk in cycle
    cyclicAtime *prev;///Prev chunk in cycle
};

///@brief scheduler working with a double linked cycle. Details see paper.
class cyclicManagedMemory : public managedMemory
{
public:
    cyclicManagedMemory ( membrain::managedSwap *swap, membrain::global_bytesize size );
    virtual ~cyclicManagedMemory();

    ///@brief prints out all elements known to the scheduler with their respective status
    void printCycle() const;
    ///@brief prints out current memory usage
    void printMemUsage() const;
    /** @brief checks whether the scheduler believes to be sane and prints an error message if not
     *  @return true if sane, false if not**/
    bool checkCycle() const;

    ///@brief sets whether scheduler should guess next needed items
    ///@param preemtive iff set to true, scheduler will guess (default)
    ///\note not thread-safe - does not make sense to call it from different threads anyway
    ///\note returns previous value
    ///@return returns previous value
    bool setPreemptiveLoading ( bool preemptive );



private:
    /** @brief cyclic implementation of swapIn, see paper
    ///\note protect call to swapIn by topologicalMutex
    **/
    virtual bool swapIn ( managedMemoryChunk &chunk );
    /** @brief cyclic implementation of swapOut, see paper
     / //*\note protect call to swapIn by topologicalMutex
     **/
    virtual swapErrorCode swapOut ( global_bytesize min_size );
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

}

#endif

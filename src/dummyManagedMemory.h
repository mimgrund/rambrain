#ifndef DUMMYMANAGEDMEMORY_H
#define DUMMYMANAGEDMEMORY_H

#include "managedMemory.h"
#include "managedDummySwap.h"

//! \todo make this dummy implementation throw stuff around when trying to use it
class dummyManagedMemory : public managedMemory
{
public:
    dummyManagedMemory() : managedMemory(new managedDummySwap(0)) {}
    ~dummyManagedMemory() {}

protected:
    inline virtual bool swapOut ( unsigned int min_size ) { return false; }
    inline virtual bool swapIn ( managedMemoryChunk &chunk ) { return false; }
    inline virtual bool touch ( managedMemoryChunk &chunk ) { return false; }
    inline virtual void schedulerRegister ( managedMemoryChunk & ) {}
    inline virtual void schedulerDelete ( managedMemoryChunk & ) {}

};

#endif // DUMMYMANAGEDMEMORY_H

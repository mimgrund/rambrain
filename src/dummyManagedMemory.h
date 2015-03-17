#ifndef DUMMYMANAGEDMEMORY_H
#define DUMMYMANAGEDMEMORY_H

#include "managedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

class dummyManagedMemory : public managedMemory
{
public:
    //! \todo How to delete the dummy swap?
    dummyManagedMemory() : managedMemory(new managedDummySwap(0)) {}
    ~dummyManagedMemory() {}

protected:
    inline virtual bool swapOut ( unsigned int  ) {
        throw dummyObjectException();
    }
    inline virtual bool swapIn ( managedMemoryChunk & ) {
        throw dummyObjectException();
    }
    inline virtual bool touch ( managedMemoryChunk & ) {
        throw dummyObjectException();
    }
    inline virtual void schedulerRegister ( managedMemoryChunk & ) {
        throw dummyObjectException();
    }
    inline virtual void schedulerDelete ( managedMemoryChunk & ) {
        throw dummyObjectException();
    }

};

#endif // DUMMYMANAGEDMEMORY_H

#ifndef DUMMYMANAGEDMEMORY_H
#define DUMMYMANAGEDMEMORY_H

#include "managedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

class dummyManagedMemory : public managedMemory
{
public:
    dummyManagedMemory() : managedMemory ( &mswap,0 ) {}
    ~dummyManagedMemory() {}

protected:
    inline virtual bool swapOut ( unsigned int  ) {
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual bool swapIn ( managedMemoryChunk & ) {
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual bool touch ( managedMemoryChunk & ) {
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual void schedulerRegister ( managedMemoryChunk & ) {
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual void schedulerDelete ( managedMemoryChunk & ) {
        throw memoryException ( "No memory manager in place." );
    }

private:
    managedDummySwap mswap = managedDummySwap ( 0 );

};

#endif // DUMMYMANAGEDMEMORY_H




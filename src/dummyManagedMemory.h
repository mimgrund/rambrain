#ifndef DUMMYMANAGEDMEMORY_H
#define DUMMYMANAGEDMEMORY_H

#include "managedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

namespace membrain
{

class dummyManagedMemory : public managedMemory
{
public:
    dummyManagedMemory() : managedMemory ( &mswap, 0 ) {}
    virtual ~dummyManagedMemory() {}

protected:
    inline virtual bool swapOut ( unsigned int  ) {
        pthread_mutex_unlock ( &managedMemory::topologicalMutex );
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual bool swapIn ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::topologicalMutex );
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual bool touch ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::topologicalMutex );
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual void schedulerRegister ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::topologicalMutex );
        throw memoryException ( "No memory manager in place." );
    }
    inline virtual void schedulerDelete ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::topologicalMutex );
        throw memoryException ( "No memory manager in place." );
    }

private:
    managedDummySwap mswap = managedDummySwap ( 0 );

};

}

#endif // DUMMYMANAGEDMEMORY_H







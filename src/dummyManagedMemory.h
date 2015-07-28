#ifndef DUMMYMANAGEDMEMORY_H
#define DUMMYMANAGEDMEMORY_H

#include "managedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

namespace rambrain
{

/** @brief a dummy managed Memory that basically does nothing and throws on everything.
 *  @note there is no productive use of this class, it only serves testing purposes
 **/
class dummyManagedMemory : public managedMemory
{
public:
    dummyManagedMemory() : managedMemory ( new managedDummySwap ( 0 ), 0 ) {}
    virtual ~dummyManagedMemory() {
        delete swap;
    }

protected:
    inline virtual swapErrorCode swapOut ( global_bytesize ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
        return ERR_SUCCESS;
    }
    inline virtual bool swapIn ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
        return ERR_SUCCESS;
    }
    inline virtual bool touch ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
        return ERR_SUCCESS;
    }
    inline virtual void schedulerRegister ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
    }
    inline virtual void schedulerDelete ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
    }

};

}

#endif // DUMMYMANAGEDMEMORY_H







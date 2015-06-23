#include "managedSwap.h"
#include "managedMemory.h"

namespace membrain
{

managedSwap::managedSwap ( global_bytesize size ) : swapSize ( size ), swapUsed ( 0 )
{
}

managedSwap::~managedSwap()
{
#ifdef SWAPSTATS
    pthread_mutex_lock ( &managedMemory::swapDeletionMutex );
#endif

    if ( managedMemory::defaultManager != NULL ) {
        if ( managedMemory::defaultManager->swap == this ) {
            managedMemory::defaultManager->swap = NULL;
        } else if ( managedMemory::defaultManager->previousManager != NULL && managedMemory::defaultManager->previousManager->swap == this ) {
            managedMemory::defaultManager->previousManager->swap = NULL;
        }
    }

#ifdef SWAPSTATS
    pthread_mutex_unlock ( &managedMemory::swapDeletionMutex );
#endif

}

}

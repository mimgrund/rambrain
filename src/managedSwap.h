#ifndef MANAGEDSWAP_H
#define MANAGEDSWAP_H

#include "managedMemoryChunk.h"
#include "common.h"
#include "managedMemory.h"
#include "membrain_atomics.h"
namespace membrain
{

class managedSwap
{
public:
    managedSwap ( global_bytesize size ) : swapSize ( size ), swapUsed ( 0 ) {}
    virtual ~managedSwap() {
        waitForCleanExit();
    }

    //Returns number of sucessfully swapped chunks
    virtual global_bytesize swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    virtual global_bytesize swapOut ( managedMemoryChunk *chunk )  = 0;
    virtual global_bytesize swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    virtual global_bytesize swapIn ( managedMemoryChunk *chunk ) = 0;
    virtual void swapDelete ( managedMemoryChunk *chunk ) = 0;

    virtual global_bytesize getSwapSize() {
        return swapSize;
    }
    virtual global_bytesize getUsedSwap() {
        return swapUsed;
    }
    virtual global_bytesize getFreeSwap() {
        return swapFree;
    }

    void claimUsageof ( global_bytesize bytes, bool rambytes, bool used ) {
        managedMemory::defaultManager->claimUsageof ( bytes, rambytes, used );
        if ( !rambytes ) {
            if ( used ) {
                membrain_atomic_fetch_sub ( &swapFree, bytes );
                membrain_atomic_fetch_add ( &swapUsed, bytes );
            } else {
                membrain_atomic_fetch_add ( &swapFree, bytes );
                membrain_atomic_fetch_sub ( &swapUsed, bytes );
            }
        }
    };
    /** Function waits for all asynchronous IO to complete.
      * The wait is implemented non-performant as a normal user does not have to wait for this.
      * Implementing this with a _cond just destroys performance in the respective swapIn/out procedures without increasing any user space functionality. **/
    void waitForCleanExit() {
        printf ( "\n" );
        while ( totalSwapActionsQueued != 0 ) {
            checkForAIO();
            printf ( "waiting for aio to complete on %d objects\r", totalSwapActionsQueued );
        };
        printf ( "                                                       \r" );
    };

    virtual bool checkForAIO() {
        return false;
    };

protected:


    global_bytesize swapSize;
    global_bytesize swapUsed;
    global_bytesize swapFree;
    unsigned int totalSwapActionsQueued = 0;
};


}




#endif







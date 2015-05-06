#ifndef MANAGEDSWAP_H
#define MANAGEDSWAP_H

#include "managedMemoryChunk.h"
#include "common.h"

namespace membrain
{

class managedSwap
{
public:
    managedSwap ( global_bytesize size ) : swapSize ( size ), swapUsed ( 0 ) {}
    //Returns number of sucessfully swapped chunks
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    virtual bool swapOut ( managedMemoryChunk *chunk )  = 0;
    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    virtual bool swapIn ( managedMemoryChunk *chunk ) = 0;
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

protected:
    global_bytesize swapSize;
    global_bytesize swapUsed;
    global_bytesize swapFree;

};

}


#endif







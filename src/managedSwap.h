#ifndef MANAGEDSWAP_H
#define MANAGEDSWAP_H

#include "managedMemoryChunk.h"
#include "common.h"

namespace membrain
{

class managedSwap
{
public:
    managedSwap ( global_bytesize size );
    virtual ~managedSwap();

    //Returns number of sucessfully swapped chunks
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    virtual bool swapOut ( managedMemoryChunk *chunk )  = 0;
    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    virtual bool swapIn ( managedMemoryChunk *chunk ) = 0;
    virtual void swapDelete ( managedMemoryChunk *chunk ) = 0;

    virtual inline global_bytesize getSwapSize() {
        return swapSize;
    }
    virtual inline global_bytesize getUsedSwap() {
        return swapUsed;
    }
    virtual inline global_bytesize getFreeSwap() {
        return swapFree;
    }

protected:
    global_bytesize swapSize;
    global_bytesize swapUsed;
    global_bytesize swapFree;
};

}


#endif







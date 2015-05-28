#ifndef MANAGEDDUMMYSWAP_H
#define MANAGEDDUMMYSWAP_H

#include "managedMemoryChunk.h"
#include "managedSwap.h"

namespace membrain
{

class managedDummySwap : public managedSwap
{
public:
    managedDummySwap ( membrain::global_bytesize size );
    virtual ~managedDummySwap() {}

    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapIn ( managedMemoryChunk *chunk );
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapOut ( managedMemoryChunk *chunk );
    virtual void swapDelete ( managedMemoryChunk *chunk );
};

}

#endif

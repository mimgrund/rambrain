#ifndef MANAGEDDUMMYSWAP_H
#define MANAGEDDUMMYSWAP_H

#include "managedMemoryChunk.h"
#include "managedSwap.h"

namespace rambrain
{

class managedDummySwap : public managedSwap
{
public:
    managedDummySwap ( rambrain::global_bytesize size );
    virtual ~managedDummySwap() {}

    virtual global_bytesize swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapIn ( managedMemoryChunk *chunk );
    virtual global_bytesize swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapOut ( managedMemoryChunk *chunk );
    virtual void swapDelete ( managedMemoryChunk *chunk );
};

}

#endif

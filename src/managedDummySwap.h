#ifndef MANAGEDDUMMYSWAP_H
#define MANAGEDDUMMYSWAP_H

#include "managedMemoryChunk.h"
#include "managedSwap.h"

namespace rambrain
{
/** @brief A dummy swap that just copies swapped out chunks to a different location in ram
 *  @note there is no productive use of this class, it simly serves testing purpuses for the managedMemory derivated classes
 **/
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

#ifndef MANAGEDDUMMYSWAP_H
#define MANAGEDDUMMYSWAP_H

#include "managedMemoryChunk.h"
#include "managedSwap.h"

class managedDummySwap : public managedSwap
{
public:
    managedDummySwap ( unsigned int size );

    virtual unsigned int swapIn ( managedMemoryChunk** chunklist, unsigned int nchunks );
    virtual bool swapIn ( managedMemoryChunk* chunk );
    virtual unsigned int swapOut ( managedMemoryChunk** chunklist, unsigned int nchunks );
    virtual bool swapOut ( managedMemoryChunk* chunk );
    virtual void swapDelete ( managedMemoryChunk *chunk );
};

#endiff
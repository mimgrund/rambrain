#ifndef MANAGEDSWAP_H
#define MANAGEDSWAP_H

#include "managedMemoryChunk.h"


class managedSwap
{
public:
    managedSwap ( unsigned int size ) : swapSize ( size ), swapUsed ( 0 ) {}
    //Returns number of sucessfully swapped chunks
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks ) =0;
    virtual bool swapOut ( managedMemoryChunk *chunk )  =0;
    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks ) =0;
    virtual bool swapIn ( managedMemoryChunk *chunk ) =0;


protected:
    unsigned int memory_swap_avail=0;
    unsigned int swapSize;
    unsigned int swapUsed;
};


#endif


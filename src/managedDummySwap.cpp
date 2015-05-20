#include <malloc.h>
#include <memory.h>
#include "managedDummySwap.h"
#include "managedMemory.h"

namespace membrain
{

managedDummySwap::managedDummySwap ( unsigned int size ) : managedSwap ( size )
{
    swapFree = swapSize = size;
    swapUsed = 0;
}

bool managedDummySwap::swapOut ( managedMemoryChunk *chunk )
{
    if ( chunk->size + swapUsed > swapSize ) {
        return false;
    }
    void *buf = malloc ( chunk->size );
    if ( buf ) {
        chunk->swapBuf = buf;
        memcpy ( chunk->swapBuf, chunk->locPtr, chunk->size );
        free ( chunk->locPtr );
        chunk->locPtr = NULL; // not strictly required.
        chunk->status = MEM_SWAPPED;
        swapUsed += chunk->size;
        swapFree -= chunk->size;
        ///We are not writing asynchronous, thus, we have to signal that we're done writing...
        managedMemory::signalSwappingCond();

        return true;
    } else {
        ///We are not writing asynchronous, thus, we have to signal that we're done writing...
        managedMemory::signalSwappingCond();
        return false;
    }


}

unsigned int managedDummySwap::swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    unsigned int n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += ( swapOut ( chunklist[n] ) ? 1 : 0 );
    }
    return n_swapped;
}


bool managedDummySwap::swapIn ( managedMemoryChunk *chunk )
{
    void *buf = malloc ( chunk->size );
    if ( buf ) {
        chunk->locPtr = buf;
        memcpy ( chunk->locPtr, chunk->swapBuf, chunk->size );
        free ( chunk->swapBuf );
        chunk->swapBuf = NULL; // Not strictly required
        chunk->status = MEM_ALLOCATED;
        swapUsed -= chunk->size;
        swapFree += chunk->size;
        ///We are not writing asynchronous, thus, we have to signal that we're done reading...
        managedMemory::signalSwappingCond();
        return true;
    } else {
        ///We are not writing asynchronous, thus, we have to signal that we're done reading...
        managedMemory::signalSwappingCond();
        return false;
    }

}


unsigned int managedDummySwap::swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    unsigned int n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += ( swapIn ( chunklist[n] ) ? 1 : 0 );
    }
    return n_swapped;
}


void managedDummySwap::swapDelete ( managedMemoryChunk *chunk )
{
    if ( chunk->status == MEM_SWAPPED ) {
        swapUsed -= chunk->size;
        swapFree += chunk->size;
        free ( chunk->swapBuf );
    }
}

}

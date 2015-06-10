#include <malloc.h>
#include <memory.h>
#include "managedDummySwap.h"
#include "managedMemory.h"

namespace membrain
{

managedDummySwap::managedDummySwap ( global_bytesize size ) : managedSwap ( size )
{
    swapFree = swapSize = size;
    swapUsed = 0;
}

global_bytesize managedDummySwap::swapOut ( managedMemoryChunk *chunk )
{
    if ( chunk->size + swapUsed > swapSize ) {
        return 0;
    }
    void *buf = malloc ( chunk->size );
    if ( buf ) {
        chunk->swapBuf = buf;
        memcpy ( chunk->swapBuf, chunk->locPtr, chunk->size );
        free ( chunk->locPtr );
        chunk->locPtr = NULL; // not strictly required.
        chunk->status = MEM_SWAPPED;
        claimUsageof ( chunk->size, false, true );
        claimUsageof ( chunk->size, true, false );
        ///We are not writing asynchronous, thus, we have to signal that we're done writing...
        managedMemory::signalSwappingCond();

        return chunk->size;
    } else {
        ///We are not writing asynchronous, thus, we have to signal that we're done writing...
        //managedMemory::signalSwappingCond();
        return 0;
    }


}

global_bytesize managedDummySwap::swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    global_bytesize n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += swapOut ( chunklist[n] );
    }
    return n_swapped;
}


global_bytesize managedDummySwap::swapIn ( managedMemoryChunk *chunk )
{
    void *buf = malloc ( chunk->size );
    if ( buf ) {
        chunk->locPtr = buf;
        memcpy ( chunk->locPtr, chunk->swapBuf, chunk->size );
        free ( chunk->swapBuf );
        chunk->swapBuf = NULL; // Not strictly required
        chunk->status = MEM_ALLOCATED;
        claimUsageof ( chunk->size, false, false );
        claimUsageof ( chunk->size, true, true );
        ///We are not writing asynchronous, thus, we have to signal that we're done reading...
        managedMemory::signalSwappingCond();
        return chunk->size;
    } else {
        ///We are not writing asynchronous, thus, we have to signal that we're done reading...
        managedMemory::signalSwappingCond();
        return 0;
    }

}


global_bytesize managedDummySwap::swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    global_bytesize n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += swapIn ( chunklist[n] );
    }
    return n_swapped;
}


void managedDummySwap::swapDelete ( managedMemoryChunk *chunk )
{
    if ( chunk->status == MEM_SWAPPED ) {
        claimUsageof ( chunk->size, false, false );
        free ( chunk->swapBuf );
    }
}

}

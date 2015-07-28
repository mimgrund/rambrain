/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <malloc.h>
#include <memory.h>
#include "managedDummySwap.h"
#include "managedMemory.h"
#include <mm_malloc.h>

namespace rambrain
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
    void *buf = _mm_malloc ( chunk->size, memoryAlignment );
    if ( buf ) {
        chunk->swapBuf = buf;
        memcpy ( chunk->swapBuf, chunk->locPtr, chunk->size );
        _mm_free ( chunk->locPtr );
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
    void *buf = _mm_malloc ( chunk->size,  memoryAlignment );
    if ( buf ) {
        chunk->locPtr = buf;
        memcpy ( chunk->locPtr, chunk->swapBuf, chunk->size );
        _mm_free ( chunk->swapBuf );
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
        _mm_free ( chunk->swapBuf );
    }
}

}


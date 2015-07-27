#include <gtest/gtest.h>
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "exceptions.h"

using namespace rambrain;

TEST ( managedDummySwap, Unit_ManualSwapping )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedDummySwap swap ( swapmem );
    cyclicManagedMemory mem ( &swap, 10 * kib ); //Need this as default memory manager will get corrupted without (memory claims...);

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = new managedMemoryChunk ( 0, 1 );
#else
    managedMemoryChunk *chunk = new managedMemoryChunk ( 1 );
#endif
    chunk->status = MEM_ALLOCATED;
    chunk->locPtr =  malloc ( dblsize );
    chunk->size = dblsize;

    swap.swapOut ( chunk );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    swap.swapIn ( chunk );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    free ( chunk->locPtr );
    delete chunk;
}

TEST ( managedDummySwap, Unit_ManualMultiSwapping )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedDummySwap swap ( swapmem );
    cyclicManagedMemory mem ( &swap, 10 * kib ); //Need this as default memory manager will get corrupted without (memory claims...);

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    managedMemoryChunk *chunks[2];
    for ( int i = 0; i < 2; ++i ) {
#ifdef PARENTAL_CONTROL
        chunks[i] = new managedMemoryChunk ( 0, i + 1 );
#else
        chunks[i] = new managedMemoryChunk ( i + 1 );
#endif
        chunks[i]->status = MEM_ALLOCATED;
        chunks[i]->locPtr =  malloc ( dblsize );
        chunks[i]->size = dblsize;
    }

    swap.swapOut ( chunks, 2 );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 2 * dblsize, swap.getUsedSwap() );

    swap.swapIn ( chunks, 2 );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    for ( int i = 0; i < 2; ++i ) {
        if ( chunks[i]->status == MEM_SWAPPED ) {
            free ( chunks[i]->swapBuf );
        } else {
            free ( chunks[i]->locPtr );
        }

        delete chunks[i];
    }
}

TEST ( managedDummySwap, Unit_ManualSwappingDelete )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedDummySwap swap ( swapmem );
    cyclicManagedMemory mem ( &swap, 10 * kib ); //Need this as default memory manager will get corrupted without (memory claims...);
    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = new managedMemoryChunk ( 0, 1 );
#else
    managedMemoryChunk *chunk = new managedMemoryChunk ( 1 );
#endif
    chunk->status = MEM_ALLOCATED;
    chunk->locPtr =  malloc ( dblsize );
    chunk->size = dblsize;

    swap.swapOut ( chunk );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    swap.swapDelete ( chunk );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    delete chunk;
}

TEST ( managedDummySwap, Unit_SwapSize )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    const unsigned int memsize = dblsize * 1.5;
    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    managedPtr<double> *ptr1 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    ptr1->setUse();

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    ptr1->unsetUse();

    managedPtr<double> *ptr2 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    ptr2->setUse();

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    ptr2->unsetUse();

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    delete ptr1;

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    delete ptr2;
}

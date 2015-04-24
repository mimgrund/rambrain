#include <gtest/gtest.h>
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "exceptions.h"

TEST (managedDummySwap, Unit_ManualSwapping )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedDummySwap swap ( swapmem );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    managedMemoryChunk* chunk = new managedMemoryChunk(0, 1);
    chunk->status = MEM_ALLOCATED;
    chunk->locPtr = new double[dblamount];
    chunk->size = dblsize;

    swap.swapOut(chunk);

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    swap.swapIn(chunk);

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    delete chunk;
}

TEST (managedDummySwap, Unit_ManualMultiSwapping )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedDummySwap swap ( swapmem );

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    managedMemoryChunk* chunks[2];
    for (int i = 0; i < 2; ++i) {
        chunks[i] = new managedMemoryChunk(0, 1);
        chunks[i]->status = MEM_ALLOCATED;
        chunks[i]->locPtr = new double[dblamount];
        chunks[i]->size = dblsize;
    }

    swap.swapOut(chunks, 2);

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 2 * dblsize, swap.getUsedSwap() );

    swap.swapIn(chunks, 2);

    ASSERT_EQ ( swapmem, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    for (int i = 0; i < 2; ++i) {
        delete chunks[i];
    }
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

#include <gtest/gtest.h>
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "exceptions.h"

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

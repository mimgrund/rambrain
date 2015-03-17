#include <gtest/gtest.h>
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "exceptions.h"

TEST ( managedSwap, Unit_SwapSize )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    const unsigned int memsize = dblsize * 1.5;
    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( 0u, swap.swapUsed );

    managedPtr<double> *ptr1 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( 0u, swap.swapUsed );

    ptr1->setUse();

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( 0u, swap.swapUsed );

    managedPtr<double> *ptr2 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( dblsize, swap.swapUsed );

    ptr2->setUse();

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( dblsize, swap.swapUsed );

    ptr2->unsetUse();

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( dblsize, swap.swapUsed );

    delete ptr2;

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( 0u, swap.swapUsed );

    ptr1->unsetUse();

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( 0u, swap.swapUsed );

    delete ptr1;
}



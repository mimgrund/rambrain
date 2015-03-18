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

    ptr1->unsetUse();

    managedPtr<double> *ptr2 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( dblsize, swap.swapUsed );

    ptr2->setUse();

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( dblsize, swap.swapUsed );

    ptr2->unsetUse();

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( dblsize, swap.swapUsed );

    delete ptr1;

    ASSERT_EQ ( swapmem, swap.swapSize );
    ASSERT_EQ ( 0u, swap.swapUsed );

    delete ptr2;
}

TEST ( managedSwap, Unit_VariousSize )
{
    const unsigned int memsize = sizeof ( double ) * 10;
    const unsigned int swapmem = 100*memsize;



    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    for ( unsigned int o=0; o<10; ++o ) {
        const unsigned int targetsize = o+1;
        managedPtr<double> *arr[8];
        for ( unsigned int p=0; p<8; ++p ) {
            arr[p] = new managedPtr<double> ( targetsize );
            ASSERT_TRUE ( manager.checkCycle() );
        }
        for ( unsigned int p=0; p<8; ++p ) {
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double,a,locA );
            locA[0] = o+p;
            ASSERT_TRUE ( manager.checkCycle() );
        }
        for ( unsigned int p=0; p<8; ++p ) {
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double,a,locA );
            EXPECT_EQ ( o+p,locA[0] );
            ASSERT_TRUE ( manager.checkCycle() );
        }
        for ( unsigned int p=0; p<8; ++p ) {
            delete arr[p];
            ASSERT_TRUE ( manager.checkCycle() );
        }
    }

}






#include <gtest/gtest.h>
#include <time.h>
#include <stdlib.h>
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "exceptions.h"

//#define SWAPSTATSLONG

class A
{
public:
    managedPtr<double> testelements = managedPtr<double> ( 10 );
    void test() {
        ADHERETO ( double, testelements );
        testelements[0] = 3;
    }
};


TEST ( cyclicManagedMemory, Unit_AllocatePointers )
{
    //Allocate Dummy swap
    managedDummySwap swap ( 100 );
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, 800 );

    //Allocate 50% of space for an double array
    managedPtr<double> gPtr ( 50 );

    //Try to rad from it
    adhereTo<double> gPtrI ( gPtr );
    double *lPtr = gPtrI;

    ASSERT_TRUE ( lPtr != NULL );

    ASSERT_EQ ( sizeof ( double ) * 50, manager.getUsedMemory() );

    ASSERT_NO_FATAL_FAILURE (
    for ( int n = 0; n < 50; n++ ) {
    lPtr[n] = 1.; //Should not segfault
    } );
}

TEST ( cyclicManagedMemory, Unit_DeepAllocatePointers )
{
    //Allocate Dummy swap
    managedDummySwap swap ( 100 );
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, 800 );


    managedPtr<A> managedA ( 2 );

    unsigned int sizeA = sizeof ( A );
    unsigned int sizeInt = sizeof ( int );
    unsigned int sizeDouble = sizeof ( double );

    ADHERETOLOC ( A, managedA, locA );

    ASSERT_TRUE ( locA != NULL );

    adhereTo<double> adhTestelements = locA->testelements;
    double *testelements = adhTestelements;
    ASSERT_TRUE ( testelements != NULL );
    ASSERT_TRUE ( manager.checkCycle() );

    locA->test();

    EXPECT_EQ ( 1u, manager.getNumberOfChildren ( managedMemory::root ) );
    EXPECT_EQ ( 2u, manager.getNumberOfChildren ( 2 ) );
    {
        managedPtr<int> test ( 10 );

        EXPECT_EQ ( 2u, manager.getNumberOfChildren ( managedMemory::root ) );
        EXPECT_EQ ( 2 * ( sizeA + 10 * sizeDouble ) + 10 * sizeInt, manager.getUsedMemory() );
    }
    EXPECT_EQ ( 2 * ( sizeA + 10 * sizeDouble ), manager.getUsedMemory() );

}

TEST ( cyclicManagedMemory, Unit_NotEnoughMemOrSwapSpace )
{
    managedDummySwap swap1 ( 0 ), swap2 ( 1000 );
    cyclicManagedMemory *manager1 = new cyclicManagedMemory ( &swap1, 0 );

    EXPECT_THROW ( managedPtr<double> ptr1 ( 10 ), memoryException );

    manager1->setMemoryLimit ( 1000 );

    EXPECT_NO_THROW ( managedPtr<double> ptr2 ( 10 ) );

    cyclicManagedMemory *manager2 = new cyclicManagedMemory ( &swap1, 90 );

    EXPECT_NO_THROW ( managedPtr<double> ptr3 ( 10 ) );
    EXPECT_THROW ( managedPtr<double> ptr5 ( 10 ); managedPtr<double> ptr4 ( 10 ), memoryException );

    manager2->setMemoryLimit ( 1000 );

    EXPECT_NO_THROW ( managedPtr<double> ptr6 ( 10 ) );

    delete manager2;
    delete manager1;
}

TEST ( cyclicManagedMemory, Integration_ArrayAccess )
{
    const  int memsize = 10240;
    const int allocarrn = 4000;
    //Allocate Dummy swap
    managedDummySwap swap ( memsize * 1000 );
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<double> *ptrs[allocarrn];
    for ( int n = 0; n < allocarrn; n++ ) {
        ptrs[n] = new managedPtr<double> ( 10 );
        ASSERT_TRUE ( manager.checkCycle() );
    }

    ASSERT_TRUE ( manager.checkCycle() );
    for ( int o = 0; o < 100; o++ ) {
        ASSERT_TRUE ( manager.checkCycle() );
        //Write fun to tree, but require swapping:
        for ( int n = 0; n < allocarrn; n++ ) {
            //ASSERT_TRUE(manager.checkCycle());
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m = 0; m < 10; m++ ) {
                darr[m] = n * 13 + m * o;
            }
        }
        //Now check equality:
        for ( int n = 0; n < allocarrn; n++ ) {
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m = 0; m < 10; m++ ) {
                EXPECT_EQ ( n * 13 + m * o, darr[m] );
            }
        }
    }
#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

    for ( int n = 0; n < allocarrn; n++ ) {
        delete ptrs[n];
    }

}


TEST ( cyclicManagedMemory, Integration_RamdomArrayAccess )
{
    const  int fac = 1000;
    const  int memsize = 10.240 * fac;
    const  int allocarrn = 4 * fac;

    //Allocate Dummy swap
    managedDummySwap swap ( allocarrn * 100 );
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<double> *ptrs[allocarrn];
    for ( int n = 0; n < allocarrn; ++n ) {
        ptrs[n] = new managedPtr<double> ( 10 );
        adhereTo<double> aLoc ( *ptrs[n] );
        double *darr = aLoc;
        for ( int m = 0; m < 10; ++m ) {
            darr[m] = -1;
        }
        ASSERT_TRUE ( manager.checkCycle() );
#ifdef SWAPSTATSLONG
        manager.printMemUsage();
#endif
    }

    ASSERT_TRUE ( manager.checkCycle() );
    for ( int o = 0; o < 10; ++o ) {
        ASSERT_TRUE ( manager.checkCycle() );
        //Write fun to tree, but require swapping:
        for ( int n = 0; n < allocarrn; ++n ) {
            unsigned int randomi = random() * allocarrn / RAND_MAX;
            adhereTo<double> aLoc ( *ptrs[randomi] );

            ASSERT_TRUE ( manager.checkCycle() );

            double *darr = aLoc;
            for ( int m = 0; m < 10; m++ ) {
                darr[m] = randomi * 13 + m * o;
            }
#ifdef SWAPSTATSLONG
            manager.printMemUsage();
#endif
        }
        //Now check equality:
        for ( int n = 0; n < allocarrn; n++ ) {
            adhereTo<double> aLoc ( *ptrs[n] );
            const double *darr = aLoc;
            for ( int m = 0; m < 10; ++m ) {
                EXPECT_TRUE ( darr[m] == -1 || ( darr[m] == n * 13 + m * o ) );

            }
#ifdef SWAPSTATSLONG
            manager.printMemUsage();
#endif
        }
        //reset numbers again:
        for ( int n = 0; n < allocarrn; ++n ) {
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m = 0; m < 10; ++m ) {
                darr[m] = -1;
            }
#ifdef SWAPSTATSLONG
            manager.printMemUsage();
#endif
        }

    }
#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

    for ( int n = 0; n < allocarrn; n++ ) {
        delete ptrs[n];
#ifdef SWAPSTATSLONG
        manager.printMemUsage();
#endif
    }

}

TEST ( cyclicManagedMemory, Unit_VariousSize )
{
    const unsigned int memsize = sizeof ( double ) * 10;
    const unsigned int swapmem = 100 * memsize;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    for ( unsigned int o = 0; o < 10; ++o ) {
        const unsigned int targetsize = o + 1;
        const unsigned int totalspace = 8 * targetsize * sizeof ( double );
        const unsigned int swappedmin = ( totalspace > memsize ? totalspace - memsize : 0u );
        managedPtr<double> *arr[8];

        for ( unsigned int p = 0; p < 8; ++p ) {
            arr[p] = new managedPtr<double> ( targetsize );
            ASSERT_TRUE ( manager.checkCycle() );
        }

        ASSERT_GE ( swap.getUsedSwap(), swappedmin );
        ASSERT_EQ ( manager.getSwappedMemory(), swap.getUsedSwap() );

        for ( unsigned int p = 0; p < 8; ++p ) {
            manager.printCycle();
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double, a, locA );
            locA[0] = o + p;
            manager.printCycle();
            ASSERT_TRUE ( manager.checkCycle() );
        }

        for ( unsigned int p = 0; p < 8; ++p ) {
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double, a, locA );
            EXPECT_EQ ( o + p, locA[0] );
            ASSERT_TRUE ( manager.checkCycle() );
        }

        for ( unsigned int p = 0; p < 8; ++p ) {
            delete arr[p];
            ASSERT_TRUE ( manager.checkCycle() );
        }

        ASSERT_EQ ( 0u, swap.getUsedSwap() );
        ASSERT_EQ ( manager.getSwappedMemory(), swap.getUsedSwap() );
    }
}

TEST ( cyclicManagedMemory, Unit_RandomAllocation )
{
    unsigned int seed = time ( NULL );
    srand ( seed );
    infomsgf ( "RNG seed is %d", seed );
    const unsigned int memsize = sizeof ( double ) * 10;
    const unsigned int swapmem = 10000 * memsize;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    for ( unsigned int o = 0; o < 10; ++o ) {
        const unsigned int arraysize = rand_r ( &seed ) / RAND_MAX * 20 + 1;
        const unsigned int targetsize = rand_r ( &seed ) / RAND_MAX * 20 + 1;
        const unsigned int totalspace = arraysize * targetsize * sizeof ( double );
        const unsigned int swappedmin = ( totalspace > memsize ? totalspace - memsize : 0u );
        managedPtr<double> *arr[arraysize];

        for ( unsigned int p = 0; p < arraysize; ++p ) {
            arr[p] = new managedPtr<double> ( targetsize );
            ASSERT_TRUE ( manager.checkCycle() );
        }

        ASSERT_GE ( swap.getUsedSwap(), swappedmin );
        ASSERT_EQ ( manager.getSwappedMemory(), swap.getUsedSwap() );

        for ( unsigned int p = 0; p < arraysize; ++p ) {
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double, a, locA );
            locA[0] = o + p;
            ASSERT_TRUE ( manager.checkCycle() );
        }

        for ( unsigned int p = 0; p < arraysize; ++p ) {
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double, a, locA );
            EXPECT_EQ ( o + p, locA[0] );
            ASSERT_TRUE ( manager.checkCycle() );
        }

        for ( unsigned int p = 0; p < arraysize; ++p ) {
            delete arr[p];
            ASSERT_TRUE ( manager.checkCycle() );
        }

        ASSERT_EQ ( 0u, swap.getUsedSwap() );
        ASSERT_EQ ( manager.getSwappedMemory(), swap.getUsedSwap() );
    }
}

TEST ( cyclicManagedMemory, Unit_MissingPointerDeallocation )
{
    ASSERT_DEATH (
        managedDummySwap swap ( 100 );
        cyclicManagedMemory manager ( &swap, 100 );
        managedPtr<double> *ptr = new managedPtr<double> ( 1 );
        , "pure virtual method called" );
}

TEST ( cyclicManagedMemory, Unit_NotEnoughSpaceForOneElement )
{
    const unsigned int memsize = sizeof ( double ) / 2;
    const unsigned int swapmem = memsize;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_THROW ( managedPtr<double> ptr ( 1 ), memoryException );
}

TEST ( cyclicManagedMemory, Unit_NotEnoughSpaceForArray )
{
    const unsigned int memsize = sizeof ( double ) * 2;
    const unsigned int swapmem = memsize;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_THROW ( managedPtr<double> ptr ( 10 ), memoryException );
}

TEST ( cyclicManagedMemory, Unit_NotEnoughSpaceInTotal )
{
    const unsigned int memsize = sizeof ( double ) * 20;
    const unsigned int swapmem = memsize;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );
    managedPtr<double> *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
    ptr1 = ptr2 = ptr3 = ptr4 = ptr5 = NULL;

    ASSERT_NO_THROW ( ptr1 = new managedPtr<double> ( 10 ) );
    ASSERT_NO_THROW ( ptr2 = new managedPtr<double> ( 10 ) );
    ASSERT_NO_THROW ( ptr3 = new managedPtr<double> ( 10 ) );
    ASSERT_NO_THROW ( ptr4 = new managedPtr<double> ( 10 ) );
    ASSERT_THROW ( ptr5 = new managedPtr<double> ( 10 ), memoryException );

    delete ptr1;
    delete ptr2;
    delete ptr3;
    delete ptr4;
    delete ptr5;
}

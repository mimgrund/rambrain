#include <gtest/gtest.h>
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
        ADHERETO ( double,testelements );
        testelements[0]=3;
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
    double * lPtr = gPtrI;

    ASSERT_TRUE ( lPtr!=NULL );

    ASSERT_EQ ( sizeof ( double ) * 50, manager.getUsedMemory() );

    ASSERT_NO_FATAL_FAILURE (
    for ( int n=0; n<50; n++ ) {
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

    ADHERETOLOC ( A,managedA,locA );

    ASSERT_TRUE ( locA != NULL );

    adhereTo<double> adhTestelements = locA->testelements;
    double * testelements = adhTestelements;
    ASSERT_TRUE ( testelements != NULL );
    ASSERT_TRUE ( manager.checkCycle() );

    locA->test();

    EXPECT_EQ ( 1, manager.getNumberOfChildren ( managedMemory::root ) );
    EXPECT_EQ ( 2, manager.getNumberOfChildren ( 2 ) );
    {
        managedPtr<int> test ( 10 );

        EXPECT_EQ ( 2, manager.getNumberOfChildren ( managedMemory::root ) );
        EXPECT_EQ ( 2*16+2*80+10*4, manager.getUsedMemory() );
    }
    EXPECT_EQ ( 2*16+2*80, manager.getUsedMemory() );

}

TEST ( cyclicManagedMemory, Unit_NotEnoughMemOrSwapSpace )
{
    managedDummySwap swap1 ( 0 ), swap2 ( 1000 );
    cyclicManagedMemory* manager1 = new cyclicManagedMemory ( &swap1, 0 );

    EXPECT_THROW ( managedPtr<double> ptr1 ( 10 ), memoryException );

    manager1->setMemoryLimit ( 1000 );

    EXPECT_NO_THROW ( managedPtr<double> ptr2 ( 10 ) );

    cyclicManagedMemory* manager2 = new cyclicManagedMemory ( &swap1, 90 );

    EXPECT_NO_THROW ( managedPtr<double> ptr3 ( 10 ) );
    EXPECT_THROW ( managedPtr<double> ptr5 ( 10 ); managedPtr<double> ptr4 ( 10 ),memoryException );

    manager2->setMemoryLimit ( 1000 );

    EXPECT_NO_THROW ( managedPtr<double> ptr6 ( 10 ) );

    delete manager2;
}

TEST ( cyclicManagedMemory, Integration_ArrayAccess )
{
    const  int memsize=10240;
    const int allocarrn = 4000;
    //Allocate Dummy swap
    managedDummySwap swap ( memsize*1000 );
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<double> *ptrs[allocarrn];
    for ( int n=0; n<allocarrn; n++ ) {
        ptrs[n] = new managedPtr<double> ( 10 );
        ASSERT_TRUE ( manager.checkCycle() );
    }

    ASSERT_TRUE ( manager.checkCycle() );
    for ( int o=0; o<100; o++ ) {
        ASSERT_TRUE ( manager.checkCycle() );
        //Write fun to tree, but require swapping:
        for ( int n=0; n<allocarrn; n++ ) {
            //ASSERT_TRUE(manager.checkCycle());
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m=0; m<10; m++ ) {
                darr[m] = n*13+m*o;
            }
        }
        //Now check equality:
        for ( int n=0; n<allocarrn; n++ ) {
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m=0; m<10; m++ ) {
                EXPECT_EQ ( n*13+m*o, darr[m] );
            }
        }
    }
#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

    for ( int n=0; n<allocarrn; n++ ) {
        delete ptrs[n];
    }

}


TEST ( cyclicManagedMemory, Integration_RamdomArrayAccess )
{
    const  int fac = 1000;
    const  int memsize=10.240*fac;
    const  int allocarrn = 4*fac;

    //Allocate Dummy swap
    managedDummySwap swap ( allocarrn*100 );
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<double> *ptrs[allocarrn];
    for ( int n=0; n<allocarrn; ++n ) {
        ptrs[n] = new managedPtr<double> ( 10 );
        adhereTo<double> aLoc ( *ptrs[n] );
        double *darr = aLoc;
        for ( int m=0; m<10; ++m ) {
            darr[m] = -1;
        }
        ASSERT_TRUE ( manager.checkCycle() );
#ifdef SWAPSTATSLONG
        manager.printMemUsage();
#endif
    }

    ASSERT_TRUE ( manager.checkCycle() );
    for ( int o=0; o<10; ++o ) {
        ASSERT_TRUE ( manager.checkCycle() );
        //Write fun to tree, but require swapping:
        for ( int n=0; n<allocarrn; ++n ) {
            unsigned int randomi = random() *allocarrn/RAND_MAX;
            adhereTo<double> aLoc ( *ptrs[randomi] );

            ASSERT_TRUE ( manager.checkCycle() );

            double *darr = aLoc;
            for ( int m=0; m<10; m++ ) {
                darr[m] = randomi*13+m*o;
            }
#ifdef SWAPSTATSLONG
            manager.printMemUsage();
#endif
        }
        //Now check equality:
        for ( int n=0; n<allocarrn; n++ ) {
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m=0; m<10; ++m ) {
                EXPECT_TRUE ( darr[m] == -1 || ( darr[m] == n*13+m*o ) );
            }
#ifdef SWAPSTATSLONG
            manager.printMemUsage();
#endif
        }
        //reset numbers again:
        for ( int n=0; n<allocarrn; ++n ) {
            adhereTo<double> aLoc ( *ptrs[n] );
            double *darr = aLoc;
            for ( int m=0; m<10; ++m ) {
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

    for ( int n=0; n<allocarrn; n++ ) {
        delete ptrs[n];
#ifdef SWAPSTATSLONG
        manager.printMemUsage();
#endif
    }

}




#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include <gtest/gtest.h>
#include "managedDummySwap.h"
int main ( int argc, char ** argv )
{
    ::testing::InitGoogleTest ( &argc,argv );
    return RUN_ALL_TESTS();
};


TEST ( cyclicManagedMemory,AllocatePointers )
{
    //Allocate Dummy swap
    managedDummySwap swap(100);
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, 800 );

    //Allocate 50% of space for an double array
    managedPtr<double> gPtr ( 50 );

    //Try to rad from it
    adhereTo<double> gPtrI ( gPtr );

    double * lPtr = gPtrI;

    ASSERT_TRUE ( lPtr!=NULL );

    ASSERT_TRUE ( manager.getUsedMemory() ==sizeof ( double ) *50 );

    for ( int n=0; n<50; n++ ) {
        lPtr[n] = 1.; //Should not segfault
    }
}




class A
{
public:
    managedPtr<double> testelements = managedPtr<double> ( 10 );
    void test();
};





void A::test()
{
    ADHERETO ( double,testelements );
    testelements[0]=3;
}

TEST ( BasicManagement,DeepAllocatePointers )
{
    //Allocate Dummy swap
    managedDummySwap swap(100);
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, 800 );


    managedPtr<A> managedA ( 2 );

    ADHERETOLOC ( A,managedA,locA );

    ASSERT_TRUE ( locA!=NULL );

    adhereTo<double> adhTestelements = locA->testelements;
    double * testelements = adhTestelements;
    ASSERT_TRUE ( testelements!=NULL );

    locA->test();


    EXPECT_TRUE ( manager.getNumberOfChildren ( managedMemory::root ) ==1 );
    EXPECT_TRUE ( manager.getNumberOfChildren ( 2 ) ==2 );
    {
        managedPtr<int> test ( 10 );

        EXPECT_TRUE ( manager.getNumberOfChildren ( managedMemory::root ) ==2 );
        EXPECT_TRUE ( manager.getUsedMemory() ==2*16+2*80+10*4 );
        manager.printTree();
    }
    EXPECT_TRUE ( manager.getUsedMemory() ==2*16+2*80 );
    manager.printTree();
};

TEST ( BasicManagement, cyclicSwappingStrategy )
{
    //Allocate Dummy swap
    managedDummySwap swap(100*1024);
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, 10024);

    managedPtr<double> *ptrs[100];
    for(int n=0; n<100; n++) {
        ptrs[n] = new managedPtr<double>(10);
    }

    //Write fun to tree, but require swapping:
    for(int n=0; n<100; n++) {
        adhereTo<double> aLoc(*ptrs[n]);
        double *darr = aLoc;
        for(int m=0; m<10; m++) {
            darr[m] = n*13+m;
        }
    }

    //Now check equality:
    for(int n=0; n<100; n++) {
        adhereTo<double> aLoc(*ptrs[n]);
        double *darr = aLoc;
        for(int m=0; m<10; m++) {
            EXPECT_EQ(darr[m],n*13+m);
        }

    }

    for(int n=0; n<100; n++) {
        delete ptrs[n];
    }

};



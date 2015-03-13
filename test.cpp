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

TEST ( cyclicManagedMemory,DeepAllocatePointers )
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
    ASSERT_TRUE(manager.checkCycle());

    locA->test();

    EXPECT_TRUE ( manager.getNumberOfChildren ( managedMemory::root ) ==1 );
    EXPECT_TRUE ( manager.getNumberOfChildren ( 2 ) ==2 );
    {
        managedPtr<int> test ( 10 );

        EXPECT_TRUE ( manager.getNumberOfChildren ( managedMemory::root ) ==2 );
        EXPECT_TRUE ( manager.getUsedMemory() ==2*16+2*80+10*4 );
    }
    EXPECT_TRUE ( manager.getUsedMemory() ==2*16+2*80 );

};

TEST ( cyclicManagedMemory, arrayAccess )
{
    const  int memsize=10240;
    const int allocarrn = 4000;
    //Allocate Dummy swap
    managedDummySwap swap(memsize*10);
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, memsize);

    managedPtr<double> *ptrs[allocarrn];
    for(int n=0; n<allocarrn; n++) {
        ptrs[n] = new managedPtr<double>(10);
        ASSERT_TRUE(manager.checkCycle());
    }

    ASSERT_TRUE(manager.checkCycle());
    for(int o=0; o<100; o++) {
        ASSERT_TRUE(manager.checkCycle());
        //Write fun to tree, but require swapping:
        for(int n=0; n<allocarrn; n++) {
            ASSERT_TRUE(manager.checkCycle());
            adhereTo<double> aLoc(*ptrs[n]);
            double *darr = aLoc;
            for(int m=0; m<10; m++) {
                darr[m] = n*13+m*o;
            }
        }
        //Now check equality:
        for(int n=0; n<allocarrn; n++) {
            adhereTo<double> aLoc(*ptrs[n]);
            double *darr = aLoc;
            for(int m=0; m<10; m++) {
                EXPECT_EQ(darr[m],n*13+m*o);
            }
        }
    }
#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

    for(int n=0; n<allocarrn; n++) {
        delete ptrs[n];
    }

};

TEST ( cyclicManagedMemory, ramdomArrayAccess )
{
    const  int memsize=10240;
    const  int allocarrn = 4000;

    //Allocate Dummy swap
    managedDummySwap swap(10*memsize);
    //Allocate Manager
    cyclicManagedMemory manager ( &swap, memsize);

    managedPtr<double> *ptrs[allocarrn];
    for(int n=0; n<allocarrn; ++n) {
        ptrs[n] = new managedPtr<double>(10);
        adhereTo<double> aLoc(*ptrs[n]);
        double *darr = aLoc;
        for(int m=0; m<10; ++m) {
            darr[m] = -1;
        }
        ASSERT_TRUE(manager.checkCycle());
    }

    ASSERT_TRUE(manager.checkCycle());
    for(int o=0; o<100; ++o) {
        ASSERT_TRUE(manager.checkCycle());
        //Write fun to tree, but require swapping:
        for(int n=0; n<allocarrn; ++n) {
            unsigned int randomi = random()*allocarrn/RAND_MAX;
            adhereTo<double> aLoc(*ptrs[randomi]);

            ASSERT_TRUE(manager.checkCycle());

            double *darr = aLoc;
            for(int m=0; m<10; m++) {
                darr[m] = randomi*13+m*o;
            }
        }
        //Now check equality:
        for(int n=0; n<allocarrn; n++) {
            adhereTo<double> aLoc(*ptrs[n]);
            double *darr = aLoc;
            for(int m=0; m<10; ++m) {
                EXPECT_TRUE(darr[m]==-1||(darr[m]==n*13+m*o));
            }
        }
        //reset numbers again:
        for(int n=0; n<allocarrn; ++n) {
            adhereTo<double> aLoc(*ptrs[n]);
            double *darr = aLoc;
            for(int m=0; m<10; ++m) {
                darr[m] = -1;
            }
        }

    }
#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

    for(int n=0; n<allocarrn; n++) {
        delete ptrs[n];
    }

};



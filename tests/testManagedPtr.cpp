#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"
#include "tester.h"

using namespace membrain;

TEST ( managedPtr, Unit_NoMemoryManager )
{
    EXPECT_NO_THROW ( managedPtr<double> ptr1 ( 10 ) );

    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );

    ASSERT_NO_THROW ( managedPtr<double> ptr2 ( 10 ) );
}


#ifdef PARENTAL_CONTROL
TEST ( managedPtr, Unit_ParentIDs )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    memoryID parent = managedMemory::defaultManager->parent;

    ASSERT_NO_THROW ( managedPtr<double> ptr ( 10 ) );
    EXPECT_EQ ( parent, managedMemory::defaultManager->parent );
}
#endif

TEST ( managedPtr, Unit_ChunkInUse )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( 10 );

    ASSERT_EQ ( MEM_ALLOCATED, ptr.chunk->status );

    ptr.setUse();

    ASSERT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );

    ptr.unsetUse();

    ASSERT_EQ ( MEM_ALLOCATED, ptr.chunk->status );

    ptr.setUse ( true );

    ASSERT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );

    ptr.unsetUse();

    ASSERT_EQ ( MEM_ALLOCATED, ptr.chunk->status );

    ptr.setUse ( false );

    ASSERT_EQ ( MEM_ALLOCATED_INUSE_READ, ptr.chunk->status );

    ptr.unsetUse();

}

TEST ( managedPtr, Unit_GetLocPointer )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( 10 );

    EXPECT_EQ ( MEM_ALLOCATED, ptr.chunk->status );
    EXPECT_THROW ( ptr.getLocPtr(), unexpectedStateException );

    ptr.setUse();

    EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );
    EXPECT_NO_THROW ( ptr.getLocPtr() );

    ptr.unsetUse();

    EXPECT_EQ ( MEM_ALLOCATED, ptr.chunk->status );
    EXPECT_THROW ( ptr.getLocPtr(), unexpectedStateException );
}


TEST ( managedPtr, Unit_SmartPointery )
{
    managedDummySwap swap ( 200 );
    cyclicManagedMemory managedMemory ( &swap, 200 );

    managedPtr<double> ptr ( 5 );
    for ( int n = 0; n < 5; n++ ) {
        EXPECT_EQ ( MEM_ALLOCATED, ptr.chunk->status );

        ADHERETOLOC ( double, ptr, lptr );
        lptr[n] = n;

        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );
    }
    managedPtr<double> ptr2 ( 5 );
    for ( int n = 0; n < 5; n++ ) {
        EXPECT_EQ ( MEM_ALLOCATED, ptr.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED, ptr2.chunk->status );

        ADHERETOLOC ( double, ptr, lptr );
        ADHERETOLOC ( double, ptr2, lptr2 );
        lptr2[n] = 1 - n;

        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr2.chunk->status );
    }

    EXPECT_EQ ( 5 * sizeof ( double ) * 2, managedMemory.getUsedMemory() );
    EXPECT_EQ ( MEM_ALLOCATED, ptr.chunk->status );
    EXPECT_EQ ( MEM_ALLOCATED, ptr2.chunk->status );

    ptr = ptr2;

    EXPECT_EQ ( 5 * sizeof ( double ), managedMemory.getUsedMemory() );
    EXPECT_EQ ( MEM_ALLOCATED, ptr.chunk->status );
    EXPECT_EQ ( MEM_ALLOCATED, ptr2.chunk->status );

    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOCCONST ( double, ptr, lptr );
        ADHERETOLOCCONST ( double, ptr2, lptr2 );

        EXPECT_EQ ( 1 - n, lptr[n] );
        EXPECT_EQ ( 1 - n, lptr2[n] );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_READ, ptr.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_READ, ptr2.chunk->status );
    }

    ptr = ptr2;

    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOC ( double, ptr, lptr );
        ADHERETOLOC ( double, ptr2, lptr2 );

        EXPECT_EQ ( 1 - n, lptr[n] );
        EXPECT_EQ ( 1 - n, lptr2[n] );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr2.chunk->status );
    }
    ptr = ptr;
    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOCCONST ( double, ptr, lptr );
        ADHERETOLOCCONST ( double, ptr2, lptr2 );

        EXPECT_EQ ( 1 - n, lptr[n] );
        EXPECT_EQ ( 1 - n, lptr2[n] );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_READ, ptr.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_READ, ptr2.chunk->status );
    }
    managedPtr<double> ptr3 ( ptr );
    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOC ( double, ptr, lptr );
        ADHERETOLOC ( double, ptr2, lptr2 );
        ADHERETOLOC ( double, ptr3, lptr3 );

        EXPECT_EQ ( 1 - n, lptr[n] );
        EXPECT_EQ ( 1 - n, lptr2[n] );
        EXPECT_EQ ( 1 - n, lptr3[n] );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr2.chunk->status );
        EXPECT_EQ ( MEM_ALLOCATED_INUSE_WRITE, ptr3.chunk->status );
    }
}


TEST ( managedPtr, Unit_DeleteWhileInUse )
{
    ///\todo This test does not work, exception is handled by libc++ instead of gtest
    /*
     * Idea: Death test, do we want to let everything die at this point?
     * Ideally, we are not allowed to throw in a destructor. Doing it anyways terminates programm
     */
    managedDummySwap swap ( 100 );
    cyclicManagedMemory *managedMemory = new cyclicManagedMemory ( &swap, 100 );
    managedPtr<double> *ptr = new managedPtr<double> ( 10 );
    ptr->setUse();

    ASSERT_DEATH ( delete ptr, "terminate called after throwing an instance of 'membrain::memoryException'" );
}


TEST ( managedPtr, Unit_DirectAccess )
{
    managedDummySwap swap ( 200 );
    cyclicManagedMemory managedMemory ( &swap, 200 );

    managedPtr<double> ptr ( 5 );
    for ( int n = 0; n < 5; n++ ) {
        ptr[n] = n;
    }

    for ( int n = 0; n < 5; n++ ) {
        EXPECT_EQ ( n, ptr[n] );
    }
}


TEST ( managedPtr, Unit_DirectAccessSwapped )
{
    const unsigned int alloc = 10u;
    const unsigned int memsize = 1.5 * alloc * sizeof ( double );
    const unsigned int swapsize = 10 * memsize;

    managedDummySwap swap ( swapsize );
    cyclicManagedMemory managedMemory ( &swap, memsize );

    managedPtr<double> ptr1 ( alloc );
    managedPtr<double> ptr2 ( alloc );
    for ( unsigned int n = 0; n < alloc; n++ ) {
        ptr1[n] = n;
        ptr2[n] = 2 * n;
    }

    for ( int n = 0; n < alloc; n++ ) {
        EXPECT_EQ ( n, ptr1[n] );
        EXPECT_EQ ( 2 * n, ptr2[n] );
    }
}


TEST ( managedPtr, Unit_CreateAndInitialize )
{

    class nurfuerspassclass
    {
    public:
        nurfuerspassclass ( int a ) : val ( a ) {};
        int val;
    };

    const unsigned int alloc = 10u;
    const unsigned int memsize = 1.5 * alloc * sizeof ( double );
    const unsigned int swapsize = 10 * memsize;

    managedDummySwap swap ( swapsize );
    cyclicManagedMemory managedMemory ( &swap, memsize );

    managedPtr<nurfuerspassclass> havefun ( 5, 5 );
    adhereTo<nurfuerspassclass> hf ( havefun );
    nurfuerspassclass *hfbuf = hf;
    for ( int n = 0; n < 5; n++ ) {
        ASSERT_EQ ( 5, hfbuf[n].val );
    }

}

TEST ( managedPtr, Unit_ZeroSizedObjects )
{
    managedDummySwap swap ( 200 );
    cyclicManagedMemory managedMemory ( &swap, 200 );
    managedPtr<bool> ptrnull ( 0 );
    managedPtr<double> ptr ( 5 );
    for ( int n = 0; n < 5; n++ ) {
        ptr[n] = n;
    }

    for ( int n = 0; n < 5; n++ ) {
        EXPECT_EQ ( n, ptr[n] );
    }
}


TEST ( managedPtr, Integration_DirectVsSmartAccess )
{
    const unsigned int alloc = 20000u;
    const unsigned int memsize = 1.5 * alloc * sizeof ( double );
    const unsigned int swapsize = 10 * memsize;
    const unsigned int runs = 10;
    tester test;
    test.startNewTimeCycle();

    managedDummySwap swap ( swapsize );
    cyclicManagedMemory managedMemory ( &swap, memsize );

    managedPtr<double> ptr1 ( alloc );
    {
        ADHERETOLOC ( double, ptr1, lptr1 );
        for ( unsigned int n = 0; n < alloc; n++ ) {
            lptr1[n] = n;
        }
    }
    managedPtr<double> ptr2 ( alloc );
    {
        ADHERETOLOC ( double, ptr2, lptr2 );
        for ( unsigned int n = 0; n < alloc; n++ ) {
            lptr2[n] = -n;
        }
    }

    test.addTimeMeasurement();

    {
        ADHERETOLOC ( double, ptr1, lptr1 );
        for ( unsigned int r = 0; r < runs; ++r ) {
            for ( unsigned int n = 0; n < alloc; n++ ) {
                EXPECT_EQ ( n, lptr1[n] );
            }
        }
    }
    {
        ADHERETOLOC ( double, ptr2, lptr2 );
        for ( unsigned int r = 0; r < runs; ++r ) {
            for ( unsigned int n = 0; n < alloc; n++ ) {
                EXPECT_EQ ( -n, lptr2[n] );
            }
        }
    }

    test.addTimeMeasurement();

    for ( unsigned int r = 0; r < runs; ++r ) {
        for ( unsigned int n = 0; n < alloc; n++ ) {
            EXPECT_EQ ( n, ptr1[n] );
            EXPECT_EQ ( -n, ptr2[n] );
        }
    }

    test.addTimeMeasurement();

    std::vector<int64_t> durations = test.getDurationsForCurrentCycle();
    infomsgf ( "Smart access ran for %d ms", durations[0] );
    infomsgf ( "Direct access ran for %d ms", durations[1] );
    infomsgf ( "Direct access cost %g as much time as smart access", 1.0 * durations[1] / durations[0] );
}

TEST ( managedPtr, Unit_MultithreadingConcurrentCreateDelete )
{
    managedDummySwap swap ( 2000 );
    cyclicManagedMemory managedMemory ( &swap, 2000 );

    const unsigned int arrsize = 200;

    managedPtr<double> *arr[arrsize];
    for ( int i = 0; i < arrsize; ++i ) {
        arr[i] = NULL;
    }
    #pragma omp parallel for
    for ( int i = 0; i < arrsize; ++i ) {
        arr[i] = new managedPtr<double> ( 1 );

    }
    #pragma omp parallel for
    for ( int i = 0; i < arrsize; ++i ) {
        delete arr[i];

    }
}

TEST ( managedPtr, DISABLED_Unit_ConcurrentUseAccess )
{
    managedDummySwap swap ( 800 );
    cyclicManagedMemory managedMemory ( &swap, 400 );

    managedPtr<double> a ( 40 );
    managedPtr<double> b ( 40 );


    #pragma omp parallel for
    for ( int i = 0; i < 1000; ++i ) {
        if ( i % 2 == 0 ) {
            ADHERETOLOC ( double, a, loc );
            loc[0] = i;

        } else {
            ADHERETOLOC ( double, b, loc );
            loc[0] = i;
        }

    }
}

TEST ( managedPtr, Unit_ConcurrentUseAccessThree )
{
    managedDummySwap swap ( 1200 );
    cyclicManagedMemory managedMemory ( &swap, 400 );
    managedMemory.setOutOfSwapIsFatal ( false );
    managedPtr<double> a[] = {managedPtr<double> ( 40 ), managedPtr<double> ( 40 ), managedPtr<double> ( 40 ) };


    #pragma omp parallel for
    for ( int i = 0; i < 10000; ++i ) {
        if ( i % 500 == 0 ) { //Don't do this all the time as parallelism is blocked by this and this is what we want to test...
            EXPECT_TRUE ( managedMemory.checkCycle() );
        }
        int idx = i % 3;
        adhereTo<double> A ( a[idx] );
        double *loc = A;
        loc[0] = i;
    }
}


class destructorTracker
{
public:
    destructorTracker() {
        ++num_instances;    // Also do something in local memory to provoke SEGV if not correctly inited
        arr[9] = 9;
    }

    ~destructorTracker() {
        --num_instances;    // Also do something in local memory to provoke SEGV if not correctly loaded
        arr[9] = 0;
    }

    static int num_instances;
private:
    double arr[10];

};
int destructorTracker::num_instances = 0;


TEST ( managedPtr, Unit_CallDestructorIfSwapped )
{
    managedDummySwap swap ( sizeof ( destructorTracker ) * 2 );
    cyclicManagedMemory managedMemory ( & swap, sizeof ( destructorTracker ) * 1.5 ) ;

    managedPtr<destructorTracker> *ptr1 = new managedPtr<destructorTracker> ( 1 );
    ASSERT_EQ ( 1, destructorTracker::num_instances );

    //Swap out first one:
    managedPtr<destructorTracker> *ptr2 = new managedPtr<destructorTracker> ( 1 );
    ASSERT_EQ ( 2, destructorTracker::num_instances );

    delete ptr2;
    ASSERT_EQ ( 1, destructorTracker::num_instances );

    delete ptr1;
    ASSERT_EQ ( 0, destructorTracker::num_instances );

}



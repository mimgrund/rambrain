/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <time.h>
#include <stdlib.h>
#include "xmmintrin.h"
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "exceptions.h"
#include <configreader.h>
#include "tester.h"
#include "rambrainconfig.h"

using namespace rambrain;

/**
* @test Checks whether we can allocate data via memory manager
*/
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

#ifdef PARENTAL_CONTROL
/**
* @test Checks whether we can allocate class objects and track the object hierarchy correctly
*/
TEST ( cyclicManagedMemory, Unit_DeepAllocatePointers )
{
    class A
    {
    public:
        managedPtr<double> testelements = managedPtr<double> ( 10 );
        void test() {
            ADHERETO ( double, testelements );
            testelements[0] = 3;
        }
    };

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
#endif

/**
* @test Checks whether we throw when there's not enough ram+swap to fullfil request
*/
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

/**
* @test Checks integrity of data written to a large array
*/
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
    EXPECT_TRUE ( manager.getHitsOverMisses() > 1. );
#endif

    for ( int n = 0; n < allocarrn; n++ ) {
        delete ptrs[n];
    }

}

/**
* @test checks integrity and accessability of array objects in random access
*/
TEST ( cyclicManagedMemory, Integration_RandomArrayAccess )
{
    const  int fac = 1000;
    const  int memsize = 10.240 * fac;
    const  int allocarrn = 4 * fac;
    tester test;
    test.setSeed();

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

#ifdef SWAPSTATS
    double averageRate = 0;
    double datain = 0, dataout = 0;
#endif
    /* //This should get us somewhere near ideal: (use for testing whether test was written correctly)
    manager.setPreemptiveLoading(false);
    manager.setPreemptiveUnloading(false);
    //*/
    ASSERT_TRUE ( manager.checkCycle() );
    for ( int o = 0; o < 10; ++o ) {
        ASSERT_TRUE ( manager.checkCycle() );
        //Write fun to tree, but require swapping:
#ifdef SWAPSTATS
        manager.resetSwapstats();
#endif

        for ( int n = 0; n < allocarrn; ++n ) {
            unsigned int randomi = test.random ( allocarrn );
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
#ifdef SWAPSTATS
        averageRate += manager.getHitsOverMisses();
        datain += manager.getTotalSwappedInBytes();
        dataout += manager.getTotalSwappedInBytes();
        manager.printSwapstats();
#endif
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
    averageRate /= 10.;
    manager.printSwapstats();
    double ideal_homirate = ( ( double ) memsize ) / ( allocarrn * 10 * sizeof ( double ) );
    infomsgf ( "Ideal hits over misses %lf vs. reached: %lf factor of ~ %lf", ideal_homirate, averageRate, ideal_homirate / averageRate );
    EXPECT_TRUE ( averageRate > .5 * ideal_homirate );

    double ideal_rate = ( 10 * allocarrn * 10 * sizeof ( double ) ) * ( 1 - ideal_homirate );
    infomsgf ( "Ideal data rate %lf vs. reached: %lf (in) %lf (out) factor of ~ %lf ", ideal_rate, datain, dataout, ( .5 * ( datain + dataout ) / ideal_rate ) );
    EXPECT_TRUE ( datain < 4 * ideal_rate );
    EXPECT_TRUE ( dataout < 4 * ideal_rate );

#endif

    for ( int n = 0; n < allocarrn; n++ ) {
        delete ptrs[n];
#ifdef SWAPSTATSLONG
        manager.printMemUsage();
#endif
    }

}

/**
* @test Checks capability to cope with objects of various size
*/
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
            managedPtr<double> &a = *arr[p];
            ADHERETOLOC ( double, a, locA );
            locA[0] = o + p;
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

/**
* @test Checks capability to cope with objects of random size in random allocation
*/
TEST ( cyclicManagedMemory, Unit_RandomAllocation )
{
    const unsigned int memsize = sizeof ( double ) * 10;
    const unsigned int swapmem = 10000 * memsize;
    tester test;
    test.setSeed();

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    for ( unsigned int o = 0; o < 10; ++o ) {
        const unsigned int arraysize = test.random ( 10 ) + 1;
        const unsigned int targetsize = test.random ( 10 ) + 1;
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

/**
* @test Checks whether we detect if a single object does not fit into ram
*/
TEST ( cyclicManagedMemory, Unit_NotEnoughSpaceForOneElement )
{
    const unsigned int memsize = sizeof ( double ) / 2;
    const unsigned int swapmem = memsize;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_THROW ( managedPtr<double> ptr ( 1 ), memoryException );
}

/**
* @test Checks whether we detect if an array object does not fit into ram
*/
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

/**
* @test Checks whether memory alignment attributes are handled correctly
*/
TEST ( cyclicManagedMemory, Unit_WorkingWithSSE )
{
    union sixteen {
        float f[4];
        __m128 simd;
    } __attribute__ ( ( aligned ( 16 ) ) );

    const unsigned int memsize = sizeof ( double ) * 3;
    const unsigned int swapmem = memsize * 100;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<sixteen> ptr1 ( 1 );
    {
        ADHERETOLOC ( sixteen, ptr1, locptr1 );
        for ( int i = 0; i < 4; ++i ) {
            locptr1[0].f[i] = i;
        }
    }

    managedPtr<sixteen> ptr2 ( 1 );
    {
        ADHERETOLOC ( sixteen, ptr2, locptr2 );
        for ( int i = 0; i < 4; ++i ) {
            locptr2[0].f[i] = i + 5;
        }
    }

    {
        ADHERETOLOC ( sixteen, ptr1, locptr1 );
        for ( int i = 0; i < 4; ++i ) {
            ASSERT_EQ ( i, locptr1[0].f[i] );
        }

        sixteen s;
        for ( int i = 0; i < 4; ++i ) {
            s.f[i] = 10 * i;
        }
        locptr1[0].simd = _mm_add_ps ( locptr1[0].simd, s.simd );
    }

    {
        ADHERETOLOCCONST ( sixteen, ptr2, locptr2 );
        for ( int i = 0; i < 4; ++i ) {
            ASSERT_EQ ( i + 5, locptr2[0].f[i] );
        }
    }

    {
        ADHERETOLOC ( sixteen, ptr1, locptr1 );
        for ( int i = 0; i < 4; ++i ) {
            ASSERT_EQ ( 11 * i, locptr1[0].f[i] );
        }
    }
}

/**
* @test Checks whether nested access in class is handled correctly
*/
TEST ( cyclicManagedMemory, Unit_HandleNestedObjects )
{
    class A
    {
    public:
        double d;
        int i;
    };

    class B
    {
    public:
        B() : a1 ( 2 ), a2 ( new managedPtr<A> ( 2 ) ) {
        }
        ~B() {
            delete a2;
        }

        void set ( int i ) {
            d1 = 10.0 * i;
            ADHERETO ( A, a1 );
            for ( int j = 0; j < 2; ++j ) {
                a1[j].d = 100.0 * i * j;
                a1[j].i = i * j;
            }
            d2 = 20.0 * i;
            adhereTo<A> a2_glue ( *a2 );
            A *loca2 = a2_glue;
            for ( int j = 0; j < 2; ++j ) {
                loca2[j].d = 1000.0 * i * j;
                loca2[j].i = 10 * i * j;
            }
            d3 = 30.0 * i;
        }

        double d1;
        managedPtr<A> a1;
        double d2;
        managedPtr<A> *a2;
        double d3;
    };


    const unsigned int memsize = ( sizeof ( B ) + sizeof ( A ) * 4 ) * 2;
    const unsigned int swapmem = memsize * 2;

    managedDummySwap swap ( swapmem );
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<B> b ( 2 );
    {
        ADHERETOLOC ( B, b, locb );
        ASSERT_NO_THROW ( locb[0].set ( 1 ) );
        ASSERT_NO_THROW ( locb[1].set ( 2 ) );
    }

    ASSERT_NO_THROW (
        managedPtr<int> i ( 5 );
        ADHERETOLOC ( int, i, loci );
    for ( int j = 0; j < 5; ++j ) {
    loci[j] = j;
    }
    );

    ASSERT_NO_THROW (
        ADHERETOLOCCONST ( B, b, locb );
    for ( int i = 0, k = 1; i < 2; ++i, ++k ) {
    ASSERT_EQ ( 10.0 * k, locb[i].d1 );

        const adhereTo<A> a1 ( locb[i].a1 );
        const A *loca1 = a1;
        for ( int j = 0; j < 2; ++j ) {
            ASSERT_EQ ( 100.0 * k * j, loca1[j].d );
            ASSERT_EQ ( k * j, loca1[j].i );
        }

        ASSERT_EQ ( 20.0 * k, locb[i].d2 );
        ASSERT_EQ ( 30.0 * k, locb[i].d3 );

        const adhereTo<A> a2 ( *locb[i].a2 );
        const A *loca2 = a2;
        for ( int j = 0; j < 2; ++j ) {
            ASSERT_EQ ( 1000.0 * k * j, loca2[j].d );
            ASSERT_EQ ( 10 * k * j, loca2[j].i );
        }
    }
    );
}

/**
* @test Checks whether forgotten pointers are garbage-collected
*/
TEST ( cyclicManagedMemory, Unit_CleanupOfForgottenPointers )
{
    const unsigned int memsize = 10 * sizeof ( double );
    const unsigned int swapmem = memsize * 10;

    managedDummySwap *swap = new managedDummySwap ( swapmem );
    cyclicManagedMemory *manager = new cyclicManagedMemory ( swap, memsize );

    managedPtr<double> *A = new managedPtr<double> ( 10 );
    {
        adhereTo<double> loca ( *A );
        double *a = loca;
        for ( int i = 0; i < 10; ++i ) {
            a[i] = 1.0 * i;
        }
    }

    managedPtr<double> *B = new managedPtr<double> ( 10 );
    {
        adhereTo<double> locb ( *B );
        double *b = locb;
        for ( int i = 0; i < 10; ++i ) {
            b[i] = 2.0 * i;
        }
    }

    A = NULL;
    B = NULL;

    ASSERT_NO_FATAL_FAILURE ( delete manager );
    ASSERT_NO_FATAL_FAILURE ( delete swap );

    //This test breaks default manager.
    rambrain::rambrainglobals::config.reinit();
}

/**
* @test Checks recovery from random access to preemptive access
* */
TEST ( cyclicManagedMemory, Unit_RecoveryFromRandomAccess )
{
    const unsigned int n_el = 1024;
    const unsigned int memsize = n_el * sizeof ( char ) / 2;
    const unsigned int swapsize = 10 * memsize;

    managedDummySwap swap ( swapsize );
    cyclicManagedMemory manager ( &swap, memsize );

    managedPtr<char> randomAccess[n_el];
    managedPtr<char> consecutiveAccess[n_el];

    tester test;
    test.setSeed();

    //Set consecutive order and check whether scheduler works correctly
    for ( int n = 0; n < n_el * 10; ++n ) {
        adhereTo<char> glue ( consecutiveAccess[n % n_el] );
        char *loc = glue;
        *loc = n % 256;
    }
    double homi_rate = manager.getHitsOverMisses();
    infomsgf ( "HoMrate = %lf ", homi_rate );
    EXPECT_TRUE ( homi_rate > 50 );
    manager.resetSwapstats();
    //Fill up preemptives with random access to object group randomAccess;
    for ( int n = 0; n < n_el * 10; ++n ) {
        unsigned int idx = test.random ( ( int ) n_el - 1 );
        adhereTo<char> glue ( randomAccess[idx] );
        char *loc = glue;
        *loc = n % 256;
    }
    homi_rate = manager.getHitsOverMisses();
    infomsgf ( "HoMrate = %lf", homi_rate );
    EXPECT_TRUE ( homi_rate < 1 );
    manager.resetSwapstats();

    //Now, try to access consecutive object group again
    for ( int n = 0; n < n_el * 10; ++n ) {
        adhereTo<char> glue ( consecutiveAccess[n % n_el] );
        char *loc = glue;
        *loc = n % 256;
    }
    //And see if we recovered from preemptives:
    homi_rate = manager.getHitsOverMisses();
    infomsgf ( "HoMrate = %lf", homi_rate );
    EXPECT_TRUE ( homi_rate > 50 );
    manager.resetSwapstats();

}




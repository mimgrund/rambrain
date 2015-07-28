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

#include "tester.h"
IGNORE_TEST_WARNINGS;

#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"
#include <omp.h>

using namespace rambrain;

/**
 * @test Checks exact loading stages of objects provoked by the use of adhereTo
 */
TEST ( adhereTo, Unit_LoadUnload )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr1 ( 2 ), ptr2 ( 2 ), ptr3 ( 2 );

    adhereTo<double> *global1 = new adhereTo<double> ( ptr1 );
    adhereTo<double> *global2 = new adhereTo<double> ( ptr2, true );
    adhereTo<double> *global3 = new adhereTo<double> ( ptr3, false );

    ASSERT_FALSE ( global1->loadedReadable );
    ASSERT_FALSE ( global1->loadedWritable );

    ASSERT_TRUE ( global2->loadedReadable );
    ASSERT_FALSE ( global2->loadedWritable );

    ASSERT_FALSE ( global3->loadedReadable );
    ASSERT_FALSE ( global3->loadedWritable );

    double *loc1 = *global1;
    double *loc2 = *global2;
    double *loc3 = *global3;

    for ( unsigned int i = 0; i < 2; ++i ) {
        ASSERT_NO_FATAL_FAILURE ( loc1[i] = i );
        ASSERT_NO_FATAL_FAILURE ( loc2[i] = i );
        ASSERT_NO_FATAL_FAILURE ( loc3[i] = i );
    }


    ASSERT_FALSE ( global1->loadedReadable );
    ASSERT_TRUE ( global1->loadedWritable );

    ASSERT_TRUE ( global2->loadedReadable );
    ASSERT_TRUE ( global2->loadedWritable );

    ASSERT_FALSE ( global3->loadedReadable );
    ASSERT_TRUE ( global3->loadedWritable );

    delete global1;
    delete global2;
    delete global3;
}

/**
 * @test Checks exact loading stages of const objects provoked by the use of adhereTo
 */
TEST ( adhereTo, Unit_LoadUnloadConst )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr1 ( 2 ), ptr2 ( 2 ), ptr3 ( 2 );

    adhereTo<double> *global1 = new adhereTo<double> ( ptr1 );
    adhereTo<double> *global2 = new adhereTo<double> ( ptr2, true );
    adhereTo<double> *global3 = new adhereTo<double> ( ptr3, false );

    ASSERT_FALSE ( global1->loadedReadable );
    ASSERT_FALSE ( global1->loadedWritable );

    ASSERT_TRUE ( global2->loadedReadable );
    ASSERT_FALSE ( global2->loadedWritable );

    ASSERT_FALSE ( global3->loadedReadable );
    ASSERT_FALSE ( global3->loadedWritable );

    const double *loc1 = *global1;
    const double *loc2 = *global2;
    const double *loc3 = *global3;
    double dummy = 0.0;

    for ( unsigned int i = 0; i < 2; ++i ) {
        ASSERT_NO_FATAL_FAILURE ( dummy = loc1[i] );
        ASSERT_NO_FATAL_FAILURE ( dummy = loc2[i] );
        ASSERT_NO_FATAL_FAILURE ( dummy = loc3[i] );
    }

    ASSERT_TRUE ( global1->loadedReadable );
    ASSERT_FALSE ( global1->loadedWritable );
    ASSERT_TRUE ( global2->loadedReadable );
    ASSERT_FALSE ( global2->loadedWritable );
    ASSERT_TRUE ( global3->loadedReadable );
    ASSERT_FALSE ( global3->loadedWritable );

    delete global1;
    delete global2;
    delete global3;
}

/**
 * @test Checks whether we can access stored data
 */
TEST ( adhereTo, Unit_AccessData )
{
    const unsigned int count = 5;
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( count );

    adhereTo<double> *global = new adhereTo<double> ( ptr );
    double *loc = *global;

    for ( unsigned int i = 0; i < count; ++i ) {
        loc[i] = i;
    }

    delete global;

    global = new adhereTo<double> ( ptr );
    loc = *global;

    for ( unsigned int i = 0; i < count; ++i ) {
        EXPECT_EQ ( i, loc[i] );
    }

    delete global;
}

/**
 * @test Checks multithreading compatibility of adhereTo
 */
TEST ( adhereTo, Unit_Multithreading )
{
    const int count = 100;
    managedDummySwap swap ( 10000 );
    cyclicManagedMemory managedMemory ( &swap, 10000 );
    managedPtr<double> ptr ( count );
    #pragma omp parallel for
    for ( int n = 0; n < count; ++n ) {
        ADHERETO ( double, ptr );
        ptr[n] = n;
    }
    ADHERETOLOC ( double, ptr, ptrloc );
    for ( int n = 0; n < count; ++n ) {
        EXPECT_EQ ( n, ptrloc[n] );
    }

    adhereTo<double> adh ( ptr );
    #pragma omp parallel for
    for ( int n = 0; n < count; ++n ) {
        double *ptr = adh;
        ptr[n] *= n;
    }
    for ( int n = 0; n < count; ++n ) {
        EXPECT_EQ ( n * n, ptrloc[n] );
    }


}

/**
 * @test Checks multithreading compatibility of adhereTo with concurrent access
 */
TEST ( adhereTo, Unit_TwiceAdhered )
{
    const unsigned int count = 5;
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( count );

    adhereTo<double> *global1 = new adhereTo<double> ( ptr );
    double *loc1 = *global1;

    ASSERT_TRUE ( global1->loadedWritable );

    for ( unsigned int i = 0; i < count; ++i ) {
        loc1[i] = i;
    }

    adhereTo<double> *global2 = NULL;
    double *loc2 = NULL;


    ASSERT_NO_THROW ( global2 = new adhereTo<double> ( ptr , true ) );
    ASSERT_TRUE ( global2->loadedReadable );
    ASSERT_NO_THROW ( loc2 = *global2 );
    ASSERT_EQ ( loc1, loc2 );

    for ( unsigned int i = 0; i < count; ++i ) {
        EXPECT_EQ ( i, loc2[i] );
    }

    ASSERT_NO_THROW ( delete global1 );

    for ( unsigned int i = 0; i < count; ++i ) {
        EXPECT_EQ ( i, loc2[i] );
    }

    ASSERT_NO_THROW ( delete global2 );
}

/**
 * @test Checks correct working of convenience macros
 */
TEST ( adhereTo, Unit_MacroUsage )
{
    const unsigned int count = 5;
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( count );

    ASSERT_NO_FATAL_FAILURE ( ADHERETOLOC ( double, ptr, loc );
    for ( unsigned int i = 0; i < count; ++i ) {
    loc[i] = i;
    } );

    ASSERT_NO_FATAL_FAILURE ( ADHERETOLOCCONST ( double, ptr, loc );
    for ( unsigned int i = 0; i < count; ++i ) {
    EXPECT_EQ ( i, loc[i] );
    } );
}

/**
 * @test Checks whether adhereTo Objects are copied correctly
 */
TEST ( adhereTo, Unit_CopyCorrectness )
{
    const unsigned int count = 5;
    managedDummySwap swap ( kib );

    cyclicManagedMemory managedMemory ( &swap, kib );
    managedPtr<double> ptr ( count );

    adhereTo<double> global1 ( ptr );
    {
        double *loc1 = global1;

        for ( unsigned int i = 0; i < count; ++i ) {
            loc1[i] = i;
        }
    }

    ASSERT_NO_THROW (
        adhereTo<double> global2 = global1;
        adhereTo<double> global3 ( global1 );

    for ( unsigned int i = 0; i < count; ++i ) {
    double *loc2 = global2;
    double *loc3 = global3;

    EXPECT_EQ ( i, loc2[i] );
        EXPECT_EQ ( i, loc3[i] );
    } );
}

RESTORE_WARNINGS;


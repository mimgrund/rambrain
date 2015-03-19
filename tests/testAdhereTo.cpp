#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"

TEST ( adhereTo, Unit_LoadUnload )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr1 ( 2 ), ptr2 ( 2 ), ptr3 ( 2 );

    adhereTo<double> *global1 = new adhereTo<double> ( ptr1 );
    adhereTo<double> *global2 = new adhereTo<double> ( ptr2, true );
    adhereTo<double> *global3 = new adhereTo<double> ( ptr3, false );

    ASSERT_FALSE ( global1->loaded );
    ASSERT_TRUE ( global2->loaded );
    ASSERT_FALSE ( global3->loaded );

    double *loc1 = *global1;
    double *loc2 = *global2;
    double *loc3 = *global3;

    for ( unsigned int i = 0; i < 2; ++i ) {
        ASSERT_NO_FATAL_FAILURE ( loc1[i] = i );
        ASSERT_NO_FATAL_FAILURE ( loc2[i] = i );
        ASSERT_NO_FATAL_FAILURE ( loc3[i] = i );
    }

    ASSERT_TRUE ( global1->loaded );
    ASSERT_TRUE ( global2->loaded );
    ASSERT_TRUE ( global3->loaded );

    delete global1;
    delete global2;
    delete global3;

}

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

TEST ( adhereTo, Unit_TwiceAdhered )
{
    const unsigned int count = 5;
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( count );

    adhereTo<double> *global1 = new adhereTo<double> ( ptr );
    double *loc1 = *global1;

    ASSERT_TRUE ( global1->loaded );

    for ( unsigned int i = 0; i < count; ++i ) {
        loc1[i] = i;
    }

    adhereTo<double> *global2 = NULL;
    double *loc2 = NULL;


    ASSERT_NO_THROW ( global2 = new adhereTo<double> ( ptr , true ) );
    ASSERT_TRUE ( global2->loaded );
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

TEST ( adhereTo, UNIT_MacroUsage )
{
    const unsigned int count = 5;
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( count );

    ASSERT_NO_FATAL_FAILURE ( ADHERETOLOC ( double, ptr, loc );
    for ( unsigned int i = 0; i < count; ++i ) {
    loc[i] = i;
    } );
}





#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

TEST ( managedPtr, Unit_NoMemoryManager )
{
    EXPECT_THROW ( managedPtr<double> ptr1 ( 10 ), memoryException );

    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );

    ASSERT_NO_THROW ( managedPtr<double> ptr2 ( 10 ) );
}

TEST ( managedPtr, Unit_ParentIDs )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    memoryID parent = managedMemory::defaultManager->parent;

    ASSERT_NO_THROW ( managedPtr<double> ptr ( 10 ) );
    EXPECT_EQ ( parent, managedMemory::defaultManager->parent );
}

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
}

TEST ( managedPtr, Unit_GetLocPointer )
{
    managedDummySwap swap ( 100 );
    cyclicManagedMemory managedMemory ( &swap, 100 );
    managedPtr<double> ptr ( 10 );

    EXPECT_THROW ( ptr.getLocPtr(), unexpectedStateException );

    ptr.setUse();

    EXPECT_NO_THROW ( ptr.getLocPtr() );

    ptr.unsetUse();
}


TEST ( managedPtr, Unit_SmartPointery )
{
    managedDummySwap swap ( 200 );
    cyclicManagedMemory managedMemory ( &swap, 200 );

    managedPtr<double> ptr ( 5 );
    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOC ( double, ptr, lptr );
        lptr[n] = n;
    }
    managedPtr<double> ptr2 ( 5 );
    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOC ( double, ptr, lptr );
        ADHERETOLOC ( double, ptr2, lptr2 );
        lptr2[n] = 1 - n;
    }
    ptr = ptr2;
    for ( int n = 0; n < 5; n++ ) {
        ADHERETOLOC ( double, ptr, lptr );
        ADHERETOLOC ( double, ptr2, lptr2 );
        EXPECT_EQ ( 1 - n, lptr[n] );
        EXPECT_EQ ( 1 - n, lptr2[n] );
    }



}


TEST ( managedPtr, Unit_DeleteWhileInUse )
{
    /* TODO This test does not work, exception is handled by libc++ instead of gtest
     * Idea: Death test, do we want to let everything die at this point?
    managedDummySwap swap(100);
    cyclicManagedMemory *managedMemory=new cyclicManagedMemory(&swap, 100);
    managedPtr<double>* ptr = new managedPtr<double>(10);
    ptr->setUse();

    EXPECT_THROW(delete ptr, memoryException);
    EXPECT_THROW(delete managedMemory, memoryException);*/

}






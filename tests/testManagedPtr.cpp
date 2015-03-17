#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"

TEST(managedPtr, Unit_NoMemoryManager)
{
    memoryID parent = managedMemory::defaultManager->parent;

    EXPECT_THROW(managedPtr<double> ptr1(10), dummyObjectException);

    managedDummySwap swap(100);
    cyclicManagedMemory managedMemory(&swap, 100);

    ASSERT_NO_THROW(managedPtr<double> ptr2(10));
}

TEST(managedPtr, Unit_ParentIDs)
{
    managedDummySwap swap(100);
    cyclicManagedMemory managedMemory(&swap, 100);
    memoryID parent = managedMemory::defaultManager->parent;

    ASSERT_NO_THROW(managedPtr<double> ptr(10));
    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

TEST(managedPtr, Unit_ChunkInUse)
{
    managedDummySwap swap(100);
    cyclicManagedMemory managedMemory(&swap, 100);
    managedPtr<double> ptr(10);

    ASSERT_EQ(MEM_ALLOCATED, ptr.chunk->status);

    ptr.setUse();

    ASSERT_EQ(MEM_ALLOCATED_INUSE, ptr.chunk->status);

    ptr.unsetUse();

    ASSERT_EQ(MEM_ALLOCATED, ptr.chunk->status);
}

TEST(managedPtr, Unit_GetLocPointer)
{
    managedDummySwap swap(100);
    cyclicManagedMemory managedMemory(&swap, 100);
    managedPtr<double> ptr(10);

    EXPECT_THROW(ptr.getLocPtr(), unexpectedStateException);

    ptr.setUse();

    EXPECT_NO_THROW(ptr.getLocPtr());

    ptr.unsetUse();
}

TEST(managedPtr, Unit_DeleteWhileInUse)
{
    managedDummySwap swap(100);
    cyclicManagedMemory managedMemory(&swap, 100);
    managedPtr<double>* ptr = new managedPtr<double>(10);
    ptr->setUse();

    EXPECT_THROW(delete ptr, memoryException);
}

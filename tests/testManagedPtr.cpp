#include <gtest/gtest.h>
#include "managedPtr.h"
#include "exceptions.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"

TEST(managedPtr, Unit_ParentIDs)
{
    memoryID parent = managedMemory::defaultManager->parent;

    EXPECT_THROW(managedPtr<double> ptr1(10), dummyObjectException);

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);

    managedDummySwap swap(100);
    cyclicManagedMemory managedMemory(&swap, 100);
    ASSERT_NO_THROW(managedPtr<double> ptr2(10));

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

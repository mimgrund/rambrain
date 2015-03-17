#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"

TEST(managedPtr, Unit_ParentIDs)
{
    managedDummySwap swap(0);
    cyclicManagedMemory mem(&swap,1024);

    memoryID parent = managedMemory::defaultManager->parent;

    managedPtr<double> ptr(10);

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

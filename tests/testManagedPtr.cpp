#include <gtest/gtest.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

TEST(managedPtr, Unit_ParentIDs)
{
    managedDummySwap swap(0);
    cyclicManagedMemory mem(&swap,1024);

    memoryID parent = managedMemory::defaultManager->parent;

    EXPECT_THROW(managedPtr<double> ptr(10), dummyObjectException);

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

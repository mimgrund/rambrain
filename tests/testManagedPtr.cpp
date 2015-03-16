#include <gtest/gtest.h>
#include "managedPtr.h"

TEST(managedPtr, ParentIDs)
{
    memoryID parent = managedMemory::defaultManager->parent;

    managedPtr<double> ptr(10);

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

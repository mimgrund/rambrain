#include <gtest/gtest.h>
#include "managedPtr.h"

TEST(managedPtr, Unit_ParentIDs)
{
    memoryID parent = managedMemory::defaultManager->parent;

    managedPtr<double> ptr(10);

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

#include <gtest/gtest.h>
#include "managedPtr.h"
#include "exceptions.h"

TEST(managedPtr, Unit_ParentIDs)
{
    memoryID parent = managedMemory::defaultManager->parent;

    EXPECT_THROW(managedPtr<double> ptr(10), dummyObjectException);

    EXPECT_EQ(parent, managedMemory::defaultManager->parent);
}

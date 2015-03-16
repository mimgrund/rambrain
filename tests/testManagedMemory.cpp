#include <gtest/gtest.h>
#include "managedMemory.h"
#include "dummyManagedMemory.h"

TEST(managedMemory, DefaultManagerPresent)
{
    ASSERT_TRUE(managedMemory::defaultManager != NULL);
}

TEST(managedMemory, DefaultManagerPresentAfterAllocation)
{
    managedMemory* mm = new dummyManagedMemory();
    managedMemory::defaultManager = mm;

    delete mm;

    ASSERT_TRUE(managedMemory::defaultManager != NULL);
}


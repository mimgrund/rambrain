#include <gtest/gtest.h>
#include "managedMemory.h"
#include "dummyManagedMemory.h"

TEST(managedMemory, DefaultManagerPresent)
{
    EXPECT_TRUE(managedMemory::defaultManager != NULL);
}

TEST(managedMemory, DefaultManagerPresentAfterAllocation)
{
    managedMemory* mm = new dummyManagedMemory();
    managedMemory::defaultManager = mm;

    delete mm;

    EXPECT_TRUE(managedMemory::defaultManager != NULL);
}

TEST(managedMemory, BaseMemoryUsage)
{
    unsigned int mem = 1000u;
    managedMemory::defaultManager->setMemoryLimit(mem);

    EXPECT_EQ(mem, managedMemory::defaultManager->getMemoryLimit());
    EXPECT_EQ(0u, managedMemory::defaultManager->getUsedMemory());
    EXPECT_EQ(0u, managedMemory::defaultManager->getSwappedMemory());
}

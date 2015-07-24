#include <gtest/gtest.h>
#include "managedMemory.h"
#include "dummyManagedMemory.h"
#include "cyclicManagedMemory.h"

using namespace membrain;
/**
* @test Checks presence of a default manager at runtime without doing anything before
*/
TEST ( managedMemory, Unit_DefaultManagerPresent )
{
    EXPECT_TRUE ( managedMemory::defaultManager != NULL );
}

/**
* @test Checks whether we fall back to default when deleting our own memory manager instance
*/
TEST ( managedMemory, Unit_DefaultManagerPresentAfterAllocation )
{
    managedMemory *mm = new dummyManagedMemory();
    delete mm;

    EXPECT_TRUE ( managedMemory::defaultManager != NULL );
}

/**
* @test Checks basic manager usage stats for consistency under resize
*/
TEST ( managedMemory, Unit_BaseMemoryUsage )
{
    unsigned int mem = 1000u;
    managedMemory::defaultManager->setMemoryLimit ( mem );

    EXPECT_EQ ( mem, managedMemory::defaultManager->getMemoryLimit() );
    EXPECT_EQ ( 0u, managedMemory::defaultManager->getUsedMemory() );
    EXPECT_EQ ( 0u, managedMemory::defaultManager->getSwappedMemory() );
}

/**
* @test prints out a version info (git version + diff)
*/
TEST ( managedMemory, DISABLED_VersionInfo )
{
    managedMemory::versionInfo();
}


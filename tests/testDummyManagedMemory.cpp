#include <gtest/gtest.h>
#include "dummyManagedMemory.h"
#include "exceptions.h"
#include "managedPtr.h"

using namespace membrain;

/**
* @test Checks that dummyManager starts crying when he is asked to create an object for us
*/
TEST ( dummyManagedMemory, Unit_ThrowsExceptions )
{
    dummyManagedMemory mgr;

    EXPECT_THROW ( managedPtr<double> notexistant ( 1 ), memoryException );
}

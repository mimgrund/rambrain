#include <gtest/gtest.h>
#include "dummyManagedMemory.h"
#include "exceptions.h"
#include "managedPtr.h"

TEST ( dummyManagedMemory, Unit_ThrowsExceptions )
{
    dummyManagedMemory mgr;

    EXPECT_THROW ( managedPtr<double> ichbinnichtexistent ( 1 ), memoryException );
}

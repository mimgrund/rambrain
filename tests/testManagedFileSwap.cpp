#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"
#include <gtest/gtest.h>

TEST ( managedFileSwap, Unit_SwapAllocation )
{
    unsigned int oneswap = 1024 * 1024 * 10;
    unsigned int totalswap = 10 * oneswap;
    managedFileSwap swap ( totalswap, "/tmp/membrainswap-%d", oneswap );
};


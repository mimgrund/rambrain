#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"
#include "managedPtr.h"
#include <gtest/gtest.h>

TEST ( managedFileSwap, Unit_SwapAllocation )
{
    unsigned int oneswap = 1024 * 1024 * 16;
    unsigned int totalswap = 16 * oneswap;
    managedFileSwap swap ( totalswap, "/tmp/membrainswap-%d", oneswap );
    cyclicManagedMemory manager ( &swap, 1024 * 10 );
    {
        managedPtr<double> testarr ( 1024 ); //80% RAM full.
        managedPtr<double> testarr2 ( 1024 ); //80% RAM full., swap out occurs.
    }



};
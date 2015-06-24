#include "managedSwap.h"
#include "managedMemory.h"

namespace membrain
{

managedSwap::managedSwap ( global_bytesize size ) : swapSize ( size ), swapUsed ( 0 )
{
}

managedSwap::~managedSwap()
{
    waitForCleanExit();
}

}

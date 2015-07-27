#include "managedSwap.h"
#include "managedMemory.h"

namespace rambrain
{

managedSwap::managedSwap ( global_bytesize size ) : swapSize ( size ), swapUsed ( 0 )
{
}

managedSwap::~managedSwap()
{
    waitForCleanExit();
}

void managedSwap::claimUsageof ( global_bytesize bytes, bool rambytes, bool used )
{
    managedMemory::defaultManager->claimUsageof ( bytes, rambytes, used );
    if ( !rambytes ) {
        swapFree += ( used ? -bytes : bytes );
        swapUsed += ( used ? bytes : -bytes );
    }
}

void managedSwap::waitForCleanExit()
{
    printf ( "\n" );
    while ( totalSwapActionsQueued != 0 ) {
        checkForAIO();
        printf ( "waiting for aio to complete on %d objects\r", totalSwapActionsQueued );
    };
    printf ( "                                                       \r" );
}

}

#include "initialisation.h"

#include "common.h"
#include "managedDummySwap.h"
#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"

namespace membrain
{
namespace membrainglobals
{
configReader config;
managedSwap *swap;
managedMemory *manager;
}
bool initialise ( unsigned int memorySize, unsigned int swapSize )
{
    bool ok = membrainglobals::config.readConfig();

    if ( !ok ) {
        warnmsg ( "Could not read config file!" );
    }

    configuration c = membrainglobals::config.getConfig();

    if ( c.swap == "managedDummySwap" ) {
        membrainglobals::swap = new managedDummySwap ( swapSize );
    } else if ( c.swap == "managedFileSwap" ) {
        membrainglobals::swap = new managedFileSwap ( swapSize, c.swapfiles.c_str() );
    }

    if ( c.memoryManager == "cyclicManagedMemory" ) {
        membrainglobals::manager = new cyclicManagedMemory ( membrainglobals::swap, memorySize );
    }


}

void cleanup()
{
    delete membrainglobals::manager;
    delete membrainglobals::swap;
}
}

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
        errmsg ( "Could not read config file!" );
    } else {
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
}

void cleanup()
{
    delete membrainglobals::swap;
    delete membrainglobals::manager;
}
}

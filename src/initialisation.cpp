#include "initialisation.h"

#include "common.h"
#include "managedDummySwap.h"
#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"

namespace membrain
{
bool initialise ( unsigned int memorySize, unsigned int swapSize )
{
    bool ok = globals::config.readConfig();

    if ( !ok ) {
        errmsg ( "Could not read config file!" );
    } else {
        configuration c = globals::config.getConfig();

        if ( c.swap == "managedDummySwap" ) {
            globals::swap = new managedDummySwap ( swapSize );
        } else if ( c.swap == "managedFileSwap" ) {
            globals::swap = new managedFileSwap ( swapSize, c.swapfiles.c_str() );
        }

        if ( c.memoryManager == "cyclicManagedMemory" ) {
            globals::manager = new cyclicManagedMemory ( globals::swap, memorySize );
        }

    }
}

void cleanup()
{
    delete globals::swap;
    delete globals::manager;
}
}

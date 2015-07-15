#include "membrainconfig.h"

#include "common.h"
#include "managedDummySwap.h"
#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "dummyManagedMemory.h"
#include "exceptions.h"

namespace membrain
{
namespace membrainglobals
{

membrainConfig::membrainConfig ()
{
    init();
}

membrainConfig::~membrainConfig()
{
    clean();
}

void membrainConfig::reinit ( bool reread )
{
    clean();
    if ( reread ) {
        bool ok = config.readConfig();

        if ( !ok ) {
            warnmsg ( "Could not read config file!" );
        }
    }
    init();
}

void membrainConfig::resizeMemory ( global_bytesize memory )
{
    configuration &c = config.getConfig();
    c.memory = memory;
    manager->setMemoryLimit ( memory );
}

void membrainConfig::resizeSwap ( global_bytesize memory )
{
    configuration &c = config.getConfig();
    c.swapMemory = memory;
    //! \todo implement a resize for the swap
    reinit ( false );
}

void membrainConfig::init ()
{
    const configuration &c = getConfig();

    if ( c.swap == "managedDummySwap" ) {
        swap = new managedDummySwap ( c.swapMemory );
    } else if ( c.swap == "managedFileSwap" ) {
        swap = new managedFileSwap ( c.swapMemory, c.swapfiles.c_str(), 0, c.enableDMA );
    }

    if ( c.memoryManager == "dummyManagedMemory" ) {
        manager = new dummyManagedMemory ( );
    } else if ( c.memoryManager == "cyclicManagedMemory" ) {
        manager = new cyclicManagedMemory ( swap, c.memory );
    }
}


void membrainConfig::clean()
{
    delete manager;
    manager = NULL;
    delete swap;
    swap = NULL;
}

}
}

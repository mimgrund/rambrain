#include "rambrainconfig.h"

#include "common.h"
#include "managedDummySwap.h"
#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "dummyManagedMemory.h"
#include "exceptions.h"

namespace rambrain
{
namespace rambrainglobals
{

rambrainConfig::rambrainConfig ()
{
    init();
}

rambrainConfig::~rambrainConfig()
{
    clean();
}

void rambrainConfig::reinit ( bool reread )
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

void rambrainConfig::resizeMemory ( global_bytesize memory )
{
    configuration &c = config.getConfig();
    c.memory.value = memory;
    manager->setMemoryLimit ( memory );
}

void rambrainConfig::resizeSwap ( global_bytesize memory )
{
    configuration &c = config.getConfig();
    c.swapMemory.value = memory;
    reinit ( false );
}

void rambrainConfig::init ()
{
    const configuration &c = getConfig();

    if ( c.swap.value == "managedDummySwap" ) {
        swap = new managedDummySwap ( c.swapMemory.value );
    } else if ( c.swap.value == "managedFileSwap" ) {
        swap = new managedFileSwap ( c.swapMemory.value, c.swapfiles.value.c_str(), 0, c.enableDMA.value );
        swap->setSwapPolicy ( c.policy.value );
    }

    if ( c.memoryManager.value == "dummyManagedMemory" ) {
        manager = new dummyManagedMemory ( );
    } else if ( c.memoryManager.value == "cyclicManagedMemory" ) {
        manager = new cyclicManagedMemory ( swap, c.memory.value );
    }
}


void rambrainConfig::clean()
{
    delete manager;
    manager = NULL;
    delete swap;
    swap = NULL;
}

}
}

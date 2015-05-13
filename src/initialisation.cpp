#include "initialisation.h"

#include "common.h"
#include "managedDummySwap.h"
#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
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

void membrainConfig::reinit()
{
    clean();
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
    throw unfinishedCodeException ( "Resize of swap memory is not implemented yet" );
}

void membrainConfig::init ()
{
    bool ok = config.readConfig();

    if ( !ok ) {
        warnmsg ( "Could not read config file!" );
    }

    configuration c = config.getConfig();

    if ( c.swap == "managedDummySwap" ) {
        swap = new managedDummySwap ( c.swapMemory );
    } else if ( c.swap == "managedFileSwap" ) {
        swap = new managedFileSwap ( c.swapMemory, c.swapfiles.c_str() );
    }

    if ( c.memoryManager == "cyclicManagedMemory" ) {
        manager = new cyclicManagedMemory ( swap, c.memory );
    }
}

void membrainConfig::clean()
{
    delete manager;
    delete swap;
}

membrainConfig config;

}
}

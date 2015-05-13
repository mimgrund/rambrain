#ifndef INITIALISATION_H
#define INITIALISATION_H

#include "configreader.h"
#include "managedSwap.h"
#include "managedMemory.h"
#include "common.h"

namespace membrain
{

namespace membrainglobals
{

class membrainConfig
{
public:
    membrainConfig();
    ~membrainConfig();

    void reinit();

    inline configuration getConfig() {
        return config.getConfig();
    }

    void resizeMemory ( global_bytesize memory );
    void resizeSwap ( global_bytesize memory );

private:
    void init();
    void clean();

    configReader config;
    managedSwap *swap;
    managedMemory *manager;

};

extern membrainConfig config;

}

}

#endif // INITIALISATION_H

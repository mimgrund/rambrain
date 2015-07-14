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

    void reinit ( bool reread = true );

    inline const configuration &getConfig() const {
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

/// You will find the object in managedMemory.cpp as we have to define it in some 'used' file in the linker sense.
extern membrainConfig config;

}

}

#endif // INITIALISATION_H

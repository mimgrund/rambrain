#ifndef INITIALISATION_H
#define INITIALISATION_H

#include "configreader.h"
#include "managedSwap.h"
#include "managedMemory.h"

namespace membrain
{

bool initialise ( unsigned int memorySize, unsigned int swapSize );
void cleanup();

namespace globals
{
configReader config;
managedSwap *swap;
managedMemory *manager;
}

}

#endif // INITIALISATION_H

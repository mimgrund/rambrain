#ifndef INITIALISATION_H
#define INITIALISATION_H

#include "configreader.h"
#include "managedSwap.h"
#include "managedMemory.h"

namespace membrain
{

bool initialise ( unsigned int memorySize, unsigned int swapSize );
void cleanup();

namespace membrainglobals
{
extern configReader config;
extern managedSwap *swap;
extern managedMemory *manager;
}

}

#endif // INITIALISATION_H

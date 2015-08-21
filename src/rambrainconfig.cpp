/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rambrainconfig.h"

#include "common.h"
#include "managedDummySwap.h"
#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "dummyManagedMemory.h"
#include "exceptions.h"
#include "git_info.h"

namespace rambrain
{
namespace rambrainglobals
{

rambrainConfig::rambrainConfig ()
{
    infomsgf ( "Greetings from Rambrain!\n\tRambrain is still in an early stage of development. Please report any strange behaviour!\n\tRambrain was compiled from git %s\tWhen reporting problens, please include this commit number.", gitCommit );
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


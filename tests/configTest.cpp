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

#include <iostream>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedDummySwap.h"
#include "rambrainconfig.h"

using namespace std;
using namespace rambrain;

/**
 * @brief A test to check if binary specific custom config files are properly read in and used.
 */
int main ( int argc, char **argv )
{
    int ret = 0;
    cout << "Starting check if binary specific config is read and used properly" << endl;

    rambrainglobals::config.setCustomConfigPath ( "../tests/testConfig.conf" );
    rambrainglobals::config.reinit ( true );

    const configuration &config = rambrainglobals::config.getConfig();
    managedMemory *man = managedMemory::defaultManager;

    if ( config.memoryManager.value != "cyclicManagedMemory" || reinterpret_cast<cyclicManagedMemory *> ( man ) == NULL ) {
        cerr << "Manager is wrong!" << endl;
        ++ ret;
    }

    /// @todo check if correct swap is in place
    if ( config.swap.value != "managedDummySwap" ) {
        cerr << "Swap is wrong!" << endl;
        ++ ret;
    }

    if ( config.memory.value != 100 || man->getMemoryLimit() != 100ul ) {
        cerr << "Memory limit is wrong! " << config.memory.value << " != " << man->getMemoryLimit() << " != 100" << endl;
        ++ ret;
    }

    /// @todo check if swap is correct size
    if ( config.swapMemory.value != 1000000000 ) {
        cerr << "Swap limit is wrong! " << config.swapMemory.value << " != 1000000000" << endl;
        ++ ret;
    }

    /// @todo also check this in system
    if ( config.swapfiles.value != "rambrainswapconfigtest-%d-%d" ) {
        cerr << "Swap files are named incorrectly! " << config.swapfiles.value << " != rambrainswapconfigtest-%d-%d" << endl;
        ++ ret;
    }

    managedPtr<double> data1 ( 10, 2.0 );
    managedPtr<double> data2 ( 10, 4.0 );

    {
        ADHERETOLOC ( double, data1, d );
        for ( int i = 0; i < 10; ++i ) {
            if ( d[i] != 2.0 ) {
                cerr << "Swapped in data is wrong at index " << i << endl;
                ++ ret;
            }
        }
    }

    cout << endl << "Done, " << ret << " errors occured" << endl;

    return ret;
}


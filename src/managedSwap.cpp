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

#include "managedSwap.h"
#include "managedMemory.h"

namespace rambrain
{

managedSwap::managedSwap ( global_bytesize size ) : swapSize ( size ), swapUsed ( 0 )
{
}

managedSwap::~managedSwap()
{
    waitForCleanExit();
}

void managedSwap::claimUsageof ( global_bytesize bytes, bool rambytes, bool used )
{
    managedMemory::defaultManager->claimUsageof ( bytes, rambytes, used );
    if ( !rambytes ) {
//         if((swapFree<bytes&&used )||swapUsed<bytes&&!used)
//             throw memoryException("Wrong memory accounting!");
        swapFree += ( used ? -bytes : bytes );
        swapUsed += ( used ? bytes : -bytes );
    }
}

void managedSwap::waitForCleanExit()
{
    printf ( "\n" );
    while ( totalSwapActionsQueued != 0 ) {
        checkForAIO();
        printf ( "waiting for aio to complete on %d objects\r", totalSwapActionsQueued );
    };
    printf ( "                                                       \r" );
}

}


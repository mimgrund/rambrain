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

#include <gtest/gtest.h>
#include "dummyManagedMemory.h"
#include "exceptions.h"
#include "managedPtr.h"

using namespace rambrain;

/**
* @test Checks that dummyManager starts crying when he is asked to create an object for us
*/
TEST ( dummyManagedMemory, Unit_ThrowsExceptions )
{
    dummyManagedMemory mgr;

    EXPECT_THROW ( managedPtr<double> notexistant ( 1 ), memoryException );
}


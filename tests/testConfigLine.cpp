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
#include "configreader.h"

using namespace std;
using namespace rambrain;

/**
* @test Checks if values with units for byte sizes can be set properly
*/
TEST ( configLine, Unit_SetValue )
{
    configLine<global_bytesize> cl1 ( "cl1", 0uLL, regexMatcher::integer );
    EXPECT_EQ ( 0uLL, cl1.value );
    cl1.setValue ( "123" );
    EXPECT_EQ ( 123uLL, cl1.value );

    configLine<global_bytesize> cl2 ( "cl2", 0uLL, regexMatcher::integer | regexMatcher::units );
    cl2.setValue ( "123 kb" );
    EXPECT_EQ ( 123 * kib, cl2.value );

    cl2.setValue ( "456 MB" );
    EXPECT_EQ ( 456 * mib, cl2.value );

    cl2.setValue ( "789 Gb" );
    EXPECT_EQ ( 789 * gig, cl2.value );
}

/**
* @test Checks if values without units can be set when units are expected
*/
TEST ( configLine, Unit_SetValueMissingUnit )
{
    configLine<global_bytesize> cl ( "cl", 0uLL, regexMatcher::integer | regexMatcher::units );
    EXPECT_EQ ( 0uLL, cl.value );
    cl.setValue ( "123" );
    EXPECT_EQ ( 123uLL, cl.value );
}


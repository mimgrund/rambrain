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
#include <fstream>
#include <ostream>
#include "configreader.h"

using namespace std;
using namespace rambrain;

/**
 * @test Checks the default values of configuration options and set up of the collection
 */
TEST ( configuration, Unit_DefaultValues )
{
    configuration config;

    ASSERT_FALSE ( config.memoryManager.value.empty() );
    ASSERT_GT ( config.memory.value, 0.0 );
    ASSERT_FALSE ( config.swap.value.empty() );
    ASSERT_FALSE ( config.swapfiles.value.empty() );
    ASSERT_GT ( config.swapMemory.value, 0.0 );
    ASSERT_FALSE ( config.enableDMA.value );
    ASSERT_EQ ( swapPolicy::autoextendable, config.policy.value );
    ASSERT_EQ ( 7u, config.configOptions.size() );
}

/**
 * @test Checks if config file in a custom path is read in properly
 */
TEST ( configReader, Unit_ParseCustomFile )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "memory = 5 GB" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_FALSE ( reader.readSuccessfully() );
    ASSERT_TRUE ( reader.readConfig() );
    ASSERT_TRUE ( reader.readSuccessfully() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
    ASSERT_EQ ( 5000000000uLL, config.memory.value );
}

/**
 * @test Checks if comment lines are ignored properly
 */
TEST ( configReader, Unit_IgnoreCommentLines )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "#memoryManager = somethingstupid" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "#memoryManager = somethingstupid" << std::endl;
    out << " # memoryManager = somethingmorestupid" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
}

/**
 * @test Checks if config variables may be missing
 */
TEST ( configReader, Unit_IgnoreMissingVariables )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_FALSE ( config.memoryManager.value.empty() );
}

/**
 * @test Checks if config variables which are missing in a specific block are overwritten by the default block
 */
TEST ( configReader, Unit_DefaultOverwritesMissingVariables )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl;
    out << "memory = 5GB" << std::endl;
    out << "[rambrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[rambrain-tests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
    ASSERT_EQ ( 5000000000uLL, config.memory.value );
}

/**
 * @test Checks if config variables which are missing in a specific block are overwritten by the default block even if the default block comes after the specific one
 */
TEST ( configReader, Unit_DefaultOverwritesMissingVariablesLookFurther )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[rambrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[rambrain-tests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[default]" << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl;
    out << "memory = 5GB" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
    ASSERT_EQ ( 5000000000uLL, config.memory.value );
}

/**
 * @test Checks if the application name can be retreived properly
 */
TEST ( configReader, Unit_ParseProgramName )
{
    configReader reader;

    ASSERT_TRUE ( reader.getApplicationName() == "rambrain-unittests" || reader.getApplicationName() == "rambrain-tests" );
}

/**
 * @test Checks if a binary specific block can overwrite the default block
 */
TEST ( configReader, Unit_OverwriteDefault )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl;
    out << "[rambrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[rambrain-tests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
}

/**
 * @test Checks if a default block after the specific block does not overwrite the latter
 */
TEST ( configReader, Unit_OverwriteDefaultInverseOrder )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[rambrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[rambrain-tests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[default]" << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
}

/**
 * @test Checks if empty lines are properly ignored
 */
TEST ( configReader, Unit_IgnoreEmptyLines )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << std::endl;
    out << "[default]" << std::endl << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl << std::endl << std::endl;
    out << "[rambrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl << std::endl;
    out << "[rambrain-tests]" << std::endl << std::endl << std::endl;;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
}


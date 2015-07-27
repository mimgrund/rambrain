#include <gtest/gtest.h>
#include <fstream>
#include <ostream>
#include "configreader.h"

using namespace std;
using namespace rambrain;

TEST ( configuration, Unit_DefaultValues )
{
    configuration config;

    ASSERT_FALSE ( config.memoryManager.value.empty() );
    ASSERT_GT ( config.memory.value, 0.0 );
    ASSERT_FALSE ( config.swap.value.empty() );
    ASSERT_FALSE ( config.swapfiles.value.empty() );
    ASSERT_GT ( config.swapMemory.value, 0.0 );
}

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

TEST ( configReader, Unit_IgnoreCommentLines )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "#memoryManager = somethingstupid" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "#memoryManager = somethingstupid" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager.value );
}

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

TEST ( configReader, Unit_ParseProgramName )
{
    configReader reader;

    ASSERT_TRUE ( reader.getApplicationName() == "rambrain-unittests" || reader.getApplicationName() == "rambrain-tests" );
}

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

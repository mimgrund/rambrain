#include <gtest/gtest.h>
#include <fstream>
#include <ostream>
#include "configreader.h"

using namespace std;
using namespace membrain;

TEST ( configReader, Unit_ParseCustomFile )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager );
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

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager );
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

    ASSERT_FALSE ( config.memoryManager.empty() );
}

TEST ( configReader, Unit_ParseProgramName )
{
    configReader reader;

    ASSERT_EQ ( "membrain-unittests", reader.getApplicationName() );
}

TEST ( configReader, Unit_OverwriteDefault )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[default]" << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl;
    out << "[membrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager );
}

TEST ( configReader, Unit_OverwriteDefaultInverseOrder )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << "[membrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl;
    out << "[default]" << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager );
}

TEST ( configReader, Unit_IgnoreNewLines )
{
    //Create custom config file
    ofstream out ( "testconfig.conf" );
    out << std::endl;
    out << "[default]" << std::endl << std::endl;
    out << "memoryManager = cyclicManagedMemory" << std::endl << std::endl << std::endl;
    out << "[membrain-unittests]" << std::endl;
    out << "memoryManager = dummyManagedMemory" << std::endl << std::endl;
    out.close();

    // Read in the file
    configReader reader;
    reader.setCustomConfigPath ( "testconfig.conf" );

    ASSERT_TRUE ( reader.readConfig() );

    configuration config = reader.getConfig();

    ASSERT_EQ ( "dummyManagedMemory", config.memoryManager );
}

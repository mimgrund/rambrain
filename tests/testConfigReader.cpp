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

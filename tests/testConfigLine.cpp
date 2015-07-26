#include <gtest/gtest.h>
#include "configreader.h"

using namespace std;
using namespace membrain;

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
    EXPECT_EQ ( 123000uLL, cl2.value );

    cl2.setValue ( "456 MB" );
    EXPECT_EQ ( 456000000uLL, cl2.value );

    cl2.setValue ( "789 Gb" );
    EXPECT_EQ ( 789000000000uLL, cl2.value );
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

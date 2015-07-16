#include <gtest/gtest.h>
#include "testFilterHandling.h"

/**
 * @brief Main method to run all unit tests registered with google test
 */
int main ( int argc, char **argv )
{
    ::testing::InitGoogleTest ( &argc, argv );

    string generalFilter = "*.Unit_*:*.DISABLED_Unit_*";
    string customFilter = ::testing::GTEST_FLAG ( filter );
    ::testing::GTEST_FLAG ( filter ) = combineFilter ( generalFilter, customFilter );

    ::testing::GTEST_FLAG ( shuffle ) =  true;

    return RUN_ALL_TESTS();
}

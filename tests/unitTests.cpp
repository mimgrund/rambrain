#include <gtest/gtest.h>
#include "testFilterHandling.h"

int main ( int argc, char **argv )
{
    ::testing::InitGoogleTest ( &argc, argv );

    string generalFilter = "*.Unit_*:*.DISABLED_Unit_*";
    string customFilter = ::testing::GTEST_FLAG ( filter );
    ::testing::GTEST_FLAG ( filter ) = combineFilter ( generalFilter, customFilter );

    return RUN_ALL_TESTS();
}

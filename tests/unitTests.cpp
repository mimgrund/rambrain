#include <gtest/gtest.h>

int main ( int argc, char **argv )
{
    ::testing::InitGoogleTest ( &argc, argv );
    ::testing::GTEST_FLAG ( filter ) = "*.Unit_*:*.DISABLED_Unit_*";
    return RUN_ALL_TESTS();
}

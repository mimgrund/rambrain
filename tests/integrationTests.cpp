#include <gtest/gtest.h>

int main ( int argc, char **argv )
{
    ::testing::InitGoogleTest ( &argc, argv );
    ::testing::GTEST_FLAG ( filter ) = "*.Integration_*:*.DISABLED_Integration_*";
    return RUN_ALL_TESTS();
}

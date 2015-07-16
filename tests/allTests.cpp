#include <gtest/gtest.h>

/**
 * @brief Main method to run all tests registered with google test
 */
int main ( int argc, char **argv )
{
    ::testing::InitGoogleTest ( &argc, argv );

    ::testing::GTEST_FLAG ( shuffle ) =  true;

    return RUN_ALL_TESTS();
}



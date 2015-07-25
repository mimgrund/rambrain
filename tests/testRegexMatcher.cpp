#include <gtest/gtest.h>
#include "regexmatcher.h"

using namespace membrain;

/**
* @test Checks if block header matching works
*/
TEST ( regexMatcher, Unit_MatchBlock )
{
    regexMatcher regex;

    EXPECT_TRUE ( regex.matchConfigBlock ( "[default]" ) );
    EXPECT_TRUE ( regex.matchConfigBlock ( "[default]", "default" ) );
    EXPECT_TRUE ( regex.matchConfigBlock ( "[bla]", "bla" ) );
    EXPECT_FALSE ( regex.matchConfigBlock ( "[bla]" ) );
    EXPECT_TRUE ( regex.matchConfigBlock ( "  [default]\t" ) );
    EXPECT_FALSE ( regex.matchConfigBlock ( "[default]:" ) );
    EXPECT_TRUE ( regex.matchConfigBlock ( "[default]\n" ) );
    EXPECT_FALSE ( regex.matchConfigBlock ( "bla[default]" ) );
    EXPECT_FALSE ( regex.matchConfigBlock ( "bla  [default]  " ) );
    EXPECT_FALSE ( regex.matchConfigBlock ( "  [default]  bla" ) );
}

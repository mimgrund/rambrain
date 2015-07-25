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

/**
 * @brief Checks if key value pair matching works
 */
TEST ( regexMatcher, Unit_KeyEqualsValue )
{
    regexMatcher regex;
    pair<string, string> kv;

    kv = regex.matchKeyEqualsValue ( "key=value" );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "value", kv.second );

    kv = regex.matchKeyEqualsValue ( " key = value " );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "value", kv.second );

    kv = regex.matchKeyEqualsValue ( "key == value" );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = key = value" );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = key value" );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key=value\n" );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "value", kv.second );

    kv = regex.matchKeyEqualsValue ( "key1 = value" );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345" );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345", kv.second );
}

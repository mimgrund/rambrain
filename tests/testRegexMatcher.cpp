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
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( " key = val%due " );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "val%due", kv.second );
}

/**
 * @brief Checks if key value pair matching works with special value types
 */
TEST ( regexMatcher, Unit_KeyEqualsSpecialValue )
{
    regexMatcher regex;
    pair<string, string> kv;

    kv = regex.matchKeyEqualsValue ( "key = 12345", regexMatcher::integer );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345", regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345.", regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345.", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 123.45", regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "123.45", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 1234.5f", regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "1234.5f", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 123.45 f", regexMatcher::floating );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345", regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345GB", regexMatcher::integer | regexMatcher::units );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345GB", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345 GB", regexMatcher::integer | regexMatcher::units );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "12345 GB", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 12345GB MB", regexMatcher::integer | regexMatcher::units );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 1.2 GB", regexMatcher::floating | regexMatcher::units );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "1.2 GB", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 123.45", regexMatcher::integer | regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "123.45", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = .123", regexMatcher::units );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = h4x", regexMatcher::integer | regexMatcher::text );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "h4x", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = true", regexMatcher::boolean );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "true", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = false", regexMatcher::boolean );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "false", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 1", regexMatcher::boolean );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = treu", regexMatcher::boolean );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 1", regexMatcher::boolean | regexMatcher::integer );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "1", kv.second );
}

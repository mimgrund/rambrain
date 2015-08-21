/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include "regexmatcher.h"

using namespace rambrain;

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

    kv = regex.matchKeyEqualsValue ( " key = val%due " );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "val%due", kv.second );

    kv = regex.matchKeyEqualsValue ( " key = files-%d_%d " );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "files-%d_%d", kv.second );

    kv = regex.matchKeyEqualsValue ( " key = value ", "key" );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "value", kv.second );

    kv = regex.matchKeyEqualsValue ( " key = value ", "mykey" );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );
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

    kv = regex.matchKeyEqualsValue ( "key = 1.2", regexMatcher::floating | regexMatcher::units );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "1.2", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = 123.45", regexMatcher::integer | regexMatcher::floating );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "123.45", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = .123", regexMatcher::units );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = h4x", regexMatcher::integer | regexMatcher::text );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "h4x", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = h4x", regexMatcher::alphanumtext );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "h4x", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = h4x", regexMatcher::integer | regexMatcher::alphanumtext );
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

    kv = regex.matchKeyEqualsValue ( "key = True", regexMatcher::boolean );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "True", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = TRUE", regexMatcher::boolean );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "TRUE", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = /bla", regexMatcher::text );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = /bla", regexMatcher::alphanumtext );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "/bla", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = \\bla", regexMatcher::text );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = \\bla", regexMatcher::alphanumtext );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "\\bla", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = ~/bla", regexMatcher::text );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = ~/bla", regexMatcher::alphanumtext );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = bla", regexMatcher::swapfilename );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = bla-%d", regexMatcher::swapfilename );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = bla_%d-%d", regexMatcher::swapfilename );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "bla_%d-%d", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = /bla/blup/.swap23_%d-%d", regexMatcher::swapfilename );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "/bla/blup/.swap23_%d-%d", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = ~/bla/blup/.swap_%d-%d", regexMatcher::swapfilename );
    EXPECT_EQ ( "key", kv.first );
    EXPECT_EQ ( "~/bla/blup/.swap_%d-%d", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = ~/bla/blup/.swap_%d-%g", regexMatcher::swapfilename );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = ~/bla/blup/.swap_%d-%d-%d", regexMatcher::swapfilename );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = ~/bla/blup/.swap_%d-%d-%g", regexMatcher::swapfilename );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );

    kv = regex.matchKeyEqualsValue ( "key = /bla/~/blup/.swap_%d-%d", regexMatcher::swapfilename );
    EXPECT_EQ ( "", kv.first );
    EXPECT_EQ ( "", kv.second );
}

/**
 * @brief Checks if double value - unit splitting works properly
 */
TEST ( regexMatcher, Unit_DoubleValueUnitSplitting )
{
    regexMatcher regex;
    pair<double, string> vu;

    vu = regex.splitDoubleValueUnit ( "1.0MB" );
    EXPECT_EQ ( 1.0, vu.first );
    EXPECT_EQ ( "MB", vu.second );

    vu = regex.splitDoubleValueUnit ( "1.0 MB" );
    EXPECT_EQ ( 1.0, vu.first );
    EXPECT_EQ ( "MB", vu.second );

    vu = regex.splitDoubleValueUnit ( "1.0f kb" );
    EXPECT_EQ ( 1.0, vu.first );
    EXPECT_EQ ( "kb", vu.second );
}

/**
 * @brief Checks if integer value - unit splitting works properly
 */
TEST ( regexMatcher, Unit_IntegerValueUnitSplitting )
{
    regexMatcher regex;
    pair<long long int, string> vu;

    vu = regex.splitDoubleValueUnit ( "1MB" );
    EXPECT_EQ ( 1LL, vu.first );
    EXPECT_EQ ( "MB", vu.second );

    vu = regex.splitDoubleValueUnit ( "1 MB" );
    EXPECT_EQ ( 1LL, vu.first );
    EXPECT_EQ ( "MB", vu.second );

    vu = regex.splitDoubleValueUnit ( "1 kb" );
    EXPECT_EQ ( 1LL, vu.first );
    EXPECT_EQ ( "kb", vu.second );
}

/**
 * @test Checks if the home directory replacing works properly
 */
TEST ( regexMatcher, Unit_SubstituteHomeDir )
{
    regexMatcher regex;

    EXPECT_EQ ( "/no/home/dir", regex.substituteHomeDir ( "/no/home/dir", "/home/user" ) );
    EXPECT_EQ ( "/home/user", regex.substituteHomeDir ( "~", "/home/user" ) );
    EXPECT_EQ ( "/home/user/bla", regex.substituteHomeDir ( "~/bla", "/home/user" ) );
    EXPECT_EQ ( "/home/user/bla//home/user/blup", regex.substituteHomeDir ( "~/bla/~/blup", "/home/user" ) );
}

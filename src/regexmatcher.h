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

#ifndef REGEXMATCHER_H
#define REGEXMATCHER_H

#include <string>
#include <map>
#include "common.h"

using namespace std;

namespace rambrain
{

/**
 * @brief Class to handle regex matching used for parsing configuration files
 */
class regexMatcher
{

public:

    /**
     * @brief An enum to caracterise what shall be matched in a specific case; flaggy
     * @warning Negative integers currently not supported!
     */
    enum matchType {
        integer = 1 << 0,
        floating = 1 << 1,
        units = 1 << 2,
        text = 1 << 3,
        alphanumtext = 1 << 4,
        boolean = 1 << 5,
        swapfilename = 1 << 6 /// @note not flaggy
    };

    /**
     * @brief Create a new regex handler, basically a dummy
     */
    regexMatcher();

    /**
     * @brief Checks if a string matches the header of a configuration block
     * @param str The source string
     * @param blockname The config block name to check for
     * @return Result of matching
     */
    bool matchConfigBlock ( const string &str, const string &blockname = "default" ) const;

    /**
     * @brief Checks if a string matches something like key = value
     * @param str The source string
     * @param valueType Which matchType the value should be, does not match otherwise
     * @return Result key and value
     */
    pair<string, string> matchKeyEqualsValue ( const string &str, int valueType = alphanumtext ) const;
    /**
     * @brief Checks if a string matches something like key = value
     * @param str The source string
     * @param key Which key to match
     * @param valueType Which matchType the value should be, does not match otherwise
     * @return Result key and value
     */
    pair<string, string> matchKeyEqualsValue ( const string &str, const string &key, int valueType = alphanumtext ) const;

    /**
     * @brief Split a string containing value and possibly unit into both parts
     * @param str The string containing double and unit
     * @return Value and unit
     */
    pair<double, string> splitDoubleValueUnit ( const string &str ) const;
    /**
     * @brief Split a string containing value and possibly unit into both parts
     * @param str The string containing integer and unit
     * @return Value and unit
     */
    pair<long long int, string> splitIntegerValueUnit ( const string &str ) const;

private:
    /**
     * @brief Create a matching regex for a certain type
     * @param type The type
     * @return The partial regex to match the type
     */
    string createRegexMatching ( int type ) const;

};

}

#endif // REGEXMATCHER_H


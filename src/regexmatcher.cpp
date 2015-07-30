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

#include "regexmatcher.h"
#include <sstream>

#ifdef USE_XPRESSIVE

#include <boost/xpressive/xpressive.hpp>
#define COMPILE_RGX(name, rx) sregex name = sregex::compile ( rx )
using namespace boost::xpressive;

#else

#include <regex>
#define COMPILE_RGX(name, rx) regex name( rx )

#endif


namespace rambrain
{

regexMatcher::regexMatcher()
{
}

bool regexMatcher::matchConfigBlock ( const string &str, const string &blockname ) const
{
    const COMPILE_RGX ( rgx, "\\s*\\[" + blockname + "\\]\\s*" );
    return regex_match ( str, rgx );
}

pair<string, string> regexMatcher::matchKeyEqualsValue ( const string &str, int valueType ) const
{
    return matchKeyEqualsValue ( str, "[a-zA-Z]+", valueType );
}

pair<string, string> regexMatcher::matchKeyEqualsValue ( const string &str, const string &key, int valueType ) const
{
    pair<string, string> res;
    smatch match;
    const COMPILE_RGX ( rgx, "\\s*(" + key + ")\\s*=\\s*(" + createRegexMatching ( valueType ) + ")\\s*" );

    if ( regex_match ( str, match, rgx ) ) {
        res.first = match[1];
        res.second = match[2];
    }

    return res;
}

string regexMatcher::createRegexMatching ( int type ) const
{
    stringstream ss;

    if ( type & boolean ) {
        ss  << "true|True|TRUE|false|False|FALSE";
        if ( type & integer ) {
            ss << "|\\d+";
        }
    } else {
        ss << "[";
        if ( type & integer || type & floating ) {
            ss << "0-9";
        }
        if ( type & text ) {
            ss << "a-zA-Z-_\\%";
        }
        ss << "]+";
        if ( type & floating ) {
            ss << "\\.?\\d*f?";
        }
        if ( type & units ) {
            ss << "\\s*[a-zA-Z]*";
        }
    }

    return ss.str();
}

pair<double, string> regexMatcher::splitDoubleValueUnit ( const string &str ) const
{
    pair<double, string> res;
    const COMPILE_RGX ( rgx, "([0-9]+\\.?\\d*f?)\\s*([a-zA-Z]*)" );
    smatch match;

    if ( regex_match ( str, match, rgx ) ) {
        res.first = atof ( static_cast<string> ( match[1] ).c_str() );
        if ( match.size() > 2 ) {
            res.second = match[2];
        }
    }

    return res;
}

pair<long long, string> regexMatcher::splitIntegerValueUnit ( const string &str ) const
{
    pair<unsigned long long, string> res;
    const COMPILE_RGX ( rgx, "([0-9]+)\\s*([a-zA-Z]*)" );
    smatch match;

    if ( regex_match ( str, match, rgx ) ) {
        res.first = atoll ( static_cast<string> ( match[1] ).c_str() );
        if ( match.length() > 2 ) {
            res.second = match[2];
        }
    }

    return res;
}

}


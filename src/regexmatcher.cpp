#include "regexmatcher.h"
#include <sstream>

namespace rambrain
{

regexMatcher::regexMatcher()
{
}

bool regexMatcher::matchConfigBlock ( const string &str, const string &blockname ) const
{
    const regex rgx ( "\\s*\\[" + blockname + "\\]\\s*" );
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
    const regex rgx ( "\\s*(" + key + ")\\s*=\\s*(" + createRegexMatching ( valueType ) + ")\\s*" );

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
    const regex rgx ( "([0-9]+\\.?\\d*f?)\\s*([a-zA-Z]*)" );
    smatch match;

    if ( regex_match ( str, match, rgx ) ) {
        res.first = atof ( static_cast<string> ( match[1] ).c_str() );
        if ( match.length() > 2 ) {
            res.second = match[2];
        }
    }

    return res;
}

pair<long long, string> regexMatcher::splitIntegerValueUnit ( const string &str ) const
{
    pair<unsigned long long, string> res;
    const regex rgx ( "([0-9]+)\\s*([a-zA-Z]*)" );
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

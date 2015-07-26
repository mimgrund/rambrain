#include "regexmatcher.h"
#include <sstream>

namespace membrain
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
        ss  << "true|false";
        if ( type & integer ) {
            ss << "|\\d+";
        }
    } else {
        ss << "[";
        if ( type & integer || type & floating ) {
            ss << "0-9";
        }
        if ( type & text ) {
            ss << "a-zA-Z\\%";
        }
        ss << "]+";
        if ( type & floating ) {
            ss << "\\.?\\d*f?";
        }
        if ( type & units ) {
            ss << "\\s*[a-zA-Z]+";
        }
    }

    return ss.str();
}

}

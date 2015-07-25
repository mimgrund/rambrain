#include "regexmatcher.h"

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

pair<string, string> regexMatcher::matchKeyEqualsValue ( const string &str ) const
{
    pair<string, string> res;
    smatch match;
    const regex rgx ( "\\s*([a-zA-Z]+)\\s*=\\s*([a-zA-Z0-9]+)\\s*" );

    if ( regex_match ( str, match, rgx ) ) {
        res.first = match[1];
        res.second = match[2];
    }

    return res;
}

}

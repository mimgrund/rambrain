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

}

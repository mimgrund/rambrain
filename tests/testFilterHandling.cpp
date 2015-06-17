#include "testFilterHandling.h"

string combineFilter ( const string &generalFilter, const string &customFilter )
{
    bool general = false;
    if ( customFilter != "*" ) {
        istringstream f ( customFilter );
        string s;
        while ( getline ( f, s, ':' ) ) {
            if ( s[0] != '+' && s[0] != '-' ) {
                general = true;
            }
        }
    }

    if ( general ) {
        return customFilter;
    } else {
        return generalFilter + ":" + customFilter;
    }
}

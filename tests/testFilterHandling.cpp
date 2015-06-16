#include "testFilterHandling.h"

string combineFilter ( const string &generalFilter, const string &customFilter )
{
    istringstream f ( customFilter );
    string s;
    bool general = false;
    while ( getline ( f, s, ':' ) ) {
        if ( s[0] != '+' && s[0] != '-' ) {
            general = true;
        }
    }

    if ( general ) {
        return customFilter;
    } else {
        return generalFilter + ":" + customFilter;
    }
}

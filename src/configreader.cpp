#include "configreader.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/statvfs.h>
#include <cstring>

namespace membrain
{

template<>
void configLine<global_bytesize>::setValue ( const string &str )
{
    if ( matchType & regexMatcher::units ) {
        regexMatcher regex;
        long long intval = 0LL;
        double dblval = 0.0;
        string unit;

        if ( matchType & regexMatcher::floating ) {
            auto splitted = regex.splitDoubleValueUnit ( str );
            intval = splitted.first;
            unit = splitted.second;

        } else {
            auto splitted = regex.splitIntegerValueUnit ( str );
            dblval = splitted.first;
            unit = splitted.second;
        }

        if ( unit == "b" || unit == "B" ) {
            value = 1uLL;
        } else if ( unit == "kb" || unit == "kB" || unit == "KB" ) {
            value = 1000uLL;
        } else if ( unit == "mb" || unit == "mB" || unit == "MB" ) {
            value = 1000000uLL;
        } else if ( unit == "gb" || unit == "gB" || unit == "gB" ) {
            value = 1000000000uLL;
        }

        value *= ( ( matchType & regexMatcher::floating ) ? dblval : intval );
    } else {
        value = atoll ( str.c_str() );
    }
}

template<>
void configLine<bool>::setValue ( const string &str )
{
    value = ! ( str == "false" || str == "False" || str == "FALSE" || str == "0" );
}

template<>
void configLine<swapPolicy>::setValue ( const string &str )
{
    if ( str == "fixed" ) {
        value = swapPolicy::fixed;
    } else if ( str == "autoextendable" ) {
        value = swapPolicy::autoextendable;
    } else if ( str == "interactive" ) {
        value = swapPolicy::interactive;
    } else {
        value = swapPolicy::autoextendable;
    }
}

configuration::configuration() : memoryManager ( "memoryManager", "cyclicManagedMemory", regexMatcher::text ),
    swap ( "swap", "managedFileSwap", regexMatcher::text ),
/** First %d will be replaced by the process id, the second one will be replaced by the swapfile id */
    swapfiles ( "swapfiles", "membrainswap-%d-%d", regexMatcher::text ),
    memory ( "memory", 0, regexMatcher::floating | regexMatcher::units ),
    swapMemory ( "swapMemory", 0, regexMatcher::floating | regexMatcher::units ),
    enableDMA ( "enableDMA", false, regexMatcher::integer | regexMatcher::boolean ),
    policy ( "policy", swapPolicy::autoextendable, regexMatcher::text )
{
    // Fill configOptions
    configOptions.push_back ( &memoryManager );
    configOptions.push_back ( &swap );
    configOptions.push_back ( &swapfiles );
    configOptions.push_back ( &memory );
    configOptions.push_back ( &swapMemory );
    configOptions.push_back ( &enableDMA );
    configOptions.push_back ( &policy );

    // Get free main memory
    ifstream meminfo ( "/proc/meminfo", ifstream::in );
    char line[1024];
    for ( int c = 0; c < 3; ++c ) {
        meminfo.getline ( line, 1024 );
    }

    global_bytesize bytes_avail;

    char *begin = NULL, *end = NULL;
    char *pos = line;
    while ( !end ) {
        if ( *pos <= '9' && *pos >= '0' ) {
            if ( begin == NULL ) {
                begin = pos;
            }
        } else {
            if ( begin != NULL ) {
                end = pos;
            }
        }
        ++pos;
    }
    * ( end + 1 ) = 0x00;
    bytes_avail = atol ( begin ) * 1024;

    memory.value = bytes_avail * 0.5;

    // Get free partition space
    char exe[1024];
    int ret;

    ret = readlink ( "/proc/self/exe", exe, sizeof ( exe ) - 1 );
    if ( ret != -1 ) {
        exe[ret] = '\0';
        struct statvfs stats;
        statvfs ( exe, &stats );

        swapMemory.value = stats.f_bfree * stats.f_bsize / 2;
    }
}

configReader::configReader()
{
}

bool configReader::readConfig()
{
    if ( !openStream() ) {
        return false;
    }

    if ( parseConfigFile() ) {
        readSuccessfullyOnce = true;
        return true;
    } else {
        readSuccessfullyOnce = false;
        return false;
    }
}

bool configReader::openStream()
{
    if ( stream.is_open() ) {
        stream.close();
    }

    stream.open ( customConfigPath );
    if ( stream.is_open() ) {
        return true;
    }

    stream.open ( localConfigPath );
    if ( stream.is_open() ) {
        return true;
    }

    stream.open ( globalConfigPath );
    if ( stream.is_open() ) {
        return true;
    }

    return false;
}

bool configReader::parseConfigFile()
{
    string line;
    bool ret = false;

    while ( stream.good() ) {
        getline ( stream, line );
        if ( regex.matchConfigBlock ( line ) ) {
            unsigned int current = stream.tellg();
            ret = parseConfigBlock();
            stream.seekg ( current );
        } else if ( regex.matchConfigBlock ( line, getApplicationName() ) ) {
            ret = parseConfigBlock();
            break;
        }
    }

    return ret;
}

bool configReader::parseConfigBlock()
{
    string line, first, value;

    while ( stream.good() ) {
        getline ( stream, line );
        first = line.substr ( 0, 1 );
        if ( first == "[" ) {
            break;
        } else if ( first == "#" ) {
            continue;
        } else {
            for ( auto it = config.configOptions.begin(); it != config.configOptions.end(); ++it ) {
                configLineBase *cl = *it;
                std::pair<string, string> match = regex.matchKeyEqualsValue ( line, cl->name, cl->matchType );
                if ( match.first == cl->name ) {
                    cl->setValue ( match.second );
                    break;
                }
            }
        }
    }

    return true;
}

string configReader::getApplicationName() const
{
    char exe[1024];
    int ret;

    ret = readlink ( "/proc/self/exe", exe, sizeof ( exe ) - 1 );
    if ( ret == -1 ) {
        return "";
    }
    exe[ret] = '\0';

    string fullName ( exe );

    unsigned int found = fullName.find_last_of ( "/\\" );
    return fullName.substr ( found + 1 );
}

}

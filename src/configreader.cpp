#include "configreader.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/statvfs.h>
#include <cstring>

namespace membrain
{

configuration::configuration()
{
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

    memory = bytes_avail * 0.5;

    // Get free partition space
    char exe[1024];
    int ret;

    ret = readlink ( "/proc/self/exe", exe, sizeof ( exe ) - 1 );
    if ( ret != -1 ) {
        exe[ret] = '\0';
        struct statvfs stats;
        statvfs ( exe, &stats );

        swapMemory = stats.f_bfree * stats.f_bsize / 2;
    }
}

configReader::configReader()
{
    // Fill config lines map
    configLines["memoryManager"] = regexMatcher::text;
    configLines["memory"] = regexMatcher::floating | regexMatcher::units;
    configLines["swap"] = regexMatcher::text;
    configLines["swapfiles"] = regexMatcher::text;
    configLines["swapMemory"] = regexMatcher::floating | regexMatcher::units;
    configLines["enableDMA"] = regexMatcher::integer | regexMatcher::boolean;
    configLines["policy"] = regexMatcher::text;
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
            for ( auto it = configLines.begin(); it != configLines.end(); ++it ) {
                std::pair<string, string> match = regex.matchKeyEqualsValue ( line, it->first, it->second );
                if ( match.first == it->first ) {
                    saveConfigOption ( match->first, match->second, it->second );
                    break;
                }
            }
        }
    }

    return true;
}

void configReader::saveConfigOption ( const string &key, const string &value, int matchType )
{
    /// @todo implement
}

swapPolicy configReader::parseSwapPolicy ( const string &line ) const
{
    if ( ! strcmp ( line.c_str(), "fixed" ) ) {
        return swapPolicy::fixed;
    } else if ( ! strcmp ( line.c_str(), "autoextendable" ) ) {
        return swapPolicy::autoextendable;
    } else if ( ! strcmp ( line.c_str(), "interactive" ) ) {
        return swapPolicy::interactive;
    } else {
        return swapPolicy::autoextendable;
    }
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

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

#include "configreader.h"

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/statvfs.h>
#include <cstring>
#include <algorithm>

namespace rambrain
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
            dblval = splitted.first;
            unit = splitted.second;

        } else {
            auto splitted = regex.splitIntegerValueUnit ( str );
            intval = splitted.first;
            unit = splitted.second;
        }

        if ( unit == "b" || unit == "B" ) {
            value = 1uLL;
        } else if ( unit == "kb" || unit == "kB" || unit == "Kb" || unit == "KB" ) {
            value = kib;
        } else if ( unit == "mb" || unit == "Mb" || unit == "MB" ) {
            value = mib;
        } else if ( unit == "gb" || unit == "Gb" || unit == "GB" ) {
            value = gig;
        } else {
            value = 1uLL;
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
    swapfiles ( "swapfiles", "rambrainswap-%d-%d", regexMatcher::swapfilename ),
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
    localConfigPath = getHomeDir() + "/.rambrain.conf";
}

bool configReader::readConfig()
{
    if ( !reopenStreams() ) {
        return false;
    }

    vector<configLineBase *> readLines;

    readSuccess = true;
    for ( int i = 0; i < 3; ++i ) {
        if ( readLines.size() < config.configOptions.size() ) {
            readSuccess &= parseConfigFile ( streams[i], readLines );
        }
    }

    config.swapfiles.setValue ( regex.substituteHomeDir ( config.swapfiles.value, getHomeDir() ) );

    return readSuccess;
}

bool configReader::reopenStreams()
{
    for ( int i = 0; i < 3; ++i ) {
        if ( streams[i].is_open() ) {
            streams[i].close();
        }
    }
    bool readAConfig = false;

    streams[0].open ( customConfigPath );
    if ( streams[0].is_open() ) {
        infomsgf ( "Rambrain is initialized using custom config file: %s\n", customConfigPath.c_str() );
        readAConfig = true;
    }
    streams[1].open ( localConfigPath );
    if ( streams[1].is_open() ) {
        infomsgf ( "Rambrain is initialized using user's config file: %s\n", localConfigPath.c_str() );
        readAConfig = true;
    }
    streams[2].open ( globalConfigPath );
    if ( streams[2].is_open() ) {
        infomsgf ( "Rambrain is initialized using system wide config file: %s\n", globalConfigPath.c_str() );
        readAConfig = true;
    }

    if ( !readAConfig ) {
        infomsg ( "Rambrain is initialized using default settings." );
    }

    return streams[0].is_open() || streams[1].is_open() || streams[2].is_open();
}

bool configReader::parseConfigFile ( istream &stream, vector<configLineBase *> &readLines )
{
    string line;
    bool ret = true, defaultDone = false, specificDone = false;
    string appName = getApplicationName();
    vector<configLineBase *> thisReadLines ( readLines );

    while ( stream.good() && ! ( defaultDone && specificDone ) ) {
        getline ( stream, line );
        stripLeadingTrailingWhitespace ( line );
        if ( line.empty() ) {
            continue;
        }

        unsigned int current = stream.tellg();

        if ( regex.matchConfigBlock ( line ) ) {
            ret &= parseConfigBlock ( stream, specificDone ? readLines : thisReadLines );

            defaultDone = true;
        } else if ( !specificDone && regex.matchConfigBlock ( line, appName ) ) {
            ret &= parseConfigBlock ( stream, readLines );

            specificDone = true;
        }

        stream.seekg ( current );
    }

    // Merge thisReadLines into readLines if a default has been done before, ergo memorize also elements from default and not specific
    if ( defaultDone ) {
        readLines.insert ( readLines.end(), thisReadLines.begin(), thisReadLines.end() );
        unique ( readLines.begin(), readLines.end() );
    }

    return ret;
}

bool configReader::parseConfigBlock ( istream &stream, vector<configLineBase *> &readLines )
{
    string line, first;

    while ( stream.good() ) {
        getline ( stream, line );
        stripLeadingTrailingWhitespace ( line );
        if ( line.empty() ) {
            continue;
        }

        first = line.substr ( 0, 1 );
        if ( first == "[" ) {
            break;
        } else if ( first == "#" ) {
            continue;
        } else {
            bool matched = false;

            for ( auto it = config.configOptions.begin(); it != config.configOptions.end(); ++it ) {
                configLineBase *cl = *it;
                std::pair<string, string> match = regex.matchKeyEqualsValue ( line, cl->name, cl->matchType );
                if ( match.first == cl->name ) {
                    matched = true;

                    if ( find ( readLines.begin(), readLines.end(), cl ) == readLines.end() ) { // Only if this config option has not been read yet
                        readLines.push_back ( cl );
                        cl->setValue ( match.second );
                        break;
                    }
                }
            }

            if ( !matched ) {
                warnmsgf ( "Could not parse config line: %s\n", line.c_str() );
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

string configReader::getHomeDir() const
{
    struct passwd *pw = getpwuid ( getuid() );
    return pw->pw_dir;
}

void configReader::stripLeadingTrailingWhitespace ( string &str ) const
{
    str.erase ( str.begin(), std::find_if ( str.begin(), str.end(), std::not1 ( std::ptr_fun<int, int> ( std::isspace ) ) ) );
    str.erase ( std::find_if ( str.rbegin(), str.rend(), std::not1 ( std::ptr_fun<int, int> ( std::isspace ) ) ).base(), str.end() );
}
}

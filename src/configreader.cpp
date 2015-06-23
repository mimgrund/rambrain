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

        swapMemory = min ( stats.f_bfree * stats.f_bsize / 2 , 100 * gig ); ///\todo overcome this limit when we've handled file handles...
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
    string applicationBlock = "[" + getApplicationName() + "]";

    while ( stream.good() ) {
        getline ( stream, line );
        if ( line == defaultBlock ) {
            unsigned int current = stream.tellg();
            ret = parseConfigBlock();
            stream.seekg ( current );
        } else if ( applicationBlock.size() > 2 && line == applicationBlock ) {
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
        } else if ( ! ( value = parseConfigLine ( line, "memoryManager" ) ).empty() ) {
            config.memoryManager = value;
        } else if ( ! ( value = parseConfigLine ( line, "swap" ) ).empty() ) {
            config.swap = value;
        } else if ( ! ( value = parseConfigLine ( line, "swapfiles" ) ).empty() ) {
            config.swapfiles = value;
        } else if ( ! ( value = parseConfigLine ( line, "memory" ) ).empty() ) {
            config.memory = atoll ( line.c_str() );
        } else if ( ! ( value = parseConfigLine ( line, "swapMemory" ) ).empty() ) {
            config.swapMemory = atoll ( line.c_str() );
        } else if ( ! ( value = parseConfigLine ( line, "enableDMA" ) ).empty() ) {
            config.enableDMA = strcmp ( line.c_str(), "true" ) == 0;
        }
    }

    return true;
}

string configReader::parseConfigLine ( const string &line, const string &key )
{
    if ( line.substr ( 0, key.length() ) == key ) {
        unsigned int pos = key.length();
        pos = line.find_first_not_of ( "= \t", pos );
        if ( pos < line.length() ) {
            return line.substr ( pos );
        }
    }

    return "";
}

string configReader::getApplicationName()
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

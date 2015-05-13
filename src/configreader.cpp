#include "configreader.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

namespace membrain
{
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

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

    return parseConfigFile();
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
            ret = parseConfigBlock();
        } else if ( applicationBlock.size() > 2 && line == applicationBlock ) {
            ret = parseConfigBlock();
            break;
        }
    }

    return ret;
}

bool configReader::parseConfigBlock()
{
    string line, value;
    int count = 0;

    while ( stream.good() ) {
        getline ( stream, line );
        if ( line.substr ( 0, 1 ) == "[" ) {
            break;
        } else if ( ! ( value = parseConfigLine ( line, "memoryManager" ) ).empty() ) {
            config.memoryManager = value;
            ++ count;
        }
    }

    return count == 1;
}

string configReader::parseConfigLine ( const string &line, const string &key )
{
    if ( line.substr ( 0, key.length() ) == key ) {
        unsigned int pos = key.length();
        pos = line.find_first_not_of ( "= \t", pos );
        if ( pos < line.length() ) {
            return line.substr ( pos + 1 );
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

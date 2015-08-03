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

#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>
#include <fstream>
#include <istream>
#include <vector>

#include "common.h"
#include "regexmatcher.h"

using namespace std;

//Test classes
#ifdef BUILD_TESTS
class configReader_Unit_ParseProgramName_Test;
#endif

namespace rambrain
{

/**
 * @brief An enumeration to regulate how the swap should define when it approaches it's set boundary
 *
 * Can either keep the boundary fixed, extend it automatically as it needs or start an interactive shell to ask the user for advice
 * @warning Currently not used
 */
enum class swapPolicy {
    fixed,
    autoextendable,
    interactive
};


/**
 * @brief Base class for config lines
 */
class configLineBase
{

public:
    /**
     * @brief Create a new config line
     * @param name It's name
     * @param matchType Which type is matches against in the config file
     */
    configLineBase ( const string &name, const int matchType ) : name ( name ), matchType ( matchType ) {}
    /**
     * @brief Destructor
     */
    virtual ~configLineBase() {}
    /**
     * @brief Reset the value from a string
     * @param str The string
     */
    virtual void setValue ( const string &str ) = 0;

    const string name;
    const int matchType;

};


/**
 * @brief Class for config key value pairs represented by a line in a config file
 */
template<typename T>
class configLine : public configLineBase
{
public:
    /**
     * @brief Create a new config option
     * @param name It's name
     * @param value It's starting value
     * @param matchType Which type is matches against in the config file
     */
    configLine ( const string &name, const T &value, const int matchType ) : configLineBase ( name, matchType ), value ( value ) {}
    /**
     * @brief Destructor
     */
    virtual ~configLine() {}

    /**
     * @brief Reset the value from a string
     * @param str The string
     */
    virtual void setValue ( const string &str ) {
        value = str;
    }

    T value;
};

/// @copydoc configLine<T>::setValue
template<>
void configLine<global_bytesize>::setValue ( const string &str );

/// @copydoc configLine<T>::setValue
template<>
void configLine<bool>::setValue ( const string &str );

/// @copydoc configLine<T>::setValue
template<>
void configLine<swapPolicy>::setValue ( const string &str );


/**
 * @brief Main struct to save configuration variables
 * @note Should not be instantiated by the user, this is done in rambrainconfig.h and managedMemory.cpp
 * @note When extended don't forget to add the new options to configOptions
 */
struct configuration {

    /**
     * @brief Init configuration with standard values using half the available memory and disk space where the binary lives
     */
    configuration();

    configLine<string> memoryManager, swap, swapfiles;
    configLine<global_bytesize> memory, swapMemory;
    configLine<bool> enableDMA;
    configLine<swapPolicy> policy;

    vector<configLineBase *> configOptions;
};


/**
 * @brief Reader class to read in and properly parse config files
 *
 * A sample config file looks like following:\n
 * [default]\n
 * key1 = value1\n
 * key2 = value2\n
 * [binname1]\n
 * key = value\n
 * ...\n
 * While keys are exactly the names of members of the configuration struct
 * There are three different locations to search for a config file, in descending priority: custom location > user home dir > global
 * Not all options need to be supplied. Missing options are taken from the default block or (if not present there either) from the defaults
 * Also all different files are taken into account to supply missing options
 * @note Comments can be inserted with leading hash
 */
class configReader
{

public:
    /**
     * @brief Init a new reader
     * @note Does not trigger a config read in since this is done lazily
     */
    configReader();

    /**
     * @brief Read in and parse config
     * @return Success
     * @see readSuccessfully
     * @see openStream
     * @see parseConfigFile
     */
    bool readConfig();

    /**
     * @brief Simple setter
     */
    inline void setCustomConfigPath ( const string &path ) {
        customConfigPath = path;
    }

    /**
     * @brief Simple getter
     */
    inline configuration &getConfig() {
        if ( !readSuccess ) {
            readConfig();
        }
        return config;
    }
    /**
     * @brief Simple getter
     */
    inline const configuration &getConfig() const {
        return config;
    }

    /**
     * @brief Simple getter
     */
    inline bool readSuccessfully() const {
        return readSuccess;
    }

private:
    /**
     * @brief (Re)Open streams to config files
     *
     * Order is Custom location > Local user space > Global config
     * @return Success
     */
    bool reopenStreams();

    /**
     * @brief Parse config file
     *
     * Look for default block as well as block matching the current binary name (which has priority of course)
     * @param stream The input stream
     * @param readLines Vector to track which config options are already read
     * @return Success
     * @see parseConfigBlock()
     */
    bool parseConfigFile ( istream &stream, vector<configLineBase *> &readLines );
    /**
     * @brief Parse an identified configuration block and extract all matching variables
     * @param stream The input stream
     * @param readLines Vector to track which config options are already read
     * @return Success
     * @warning Does not check for unknown variables or typos, every unknown thing is simply ignored
     * @see parseConfigLine
     */
    bool parseConfigBlock ( istream &stream, vector<configLineBase *> &readLines );

    /**
     * @brief Extract the current binary's name out of the /proc file system
     * @return The name
     */
    string getApplicationName() const;

    const string globalConfigPath = "/etc/rambrain.conf";
    const string localConfigPath = "~/.rambrain.conf";
    string customConfigPath = "";

    const regexMatcher regex;

    ifstream streams[3];

    configuration config;

    bool readSuccess = false;

    //Test classes
#ifdef BUILD_TESTS
    friend class ::configReader_Unit_ParseProgramName_Test;
#endif

};

}

#endif // CONFIGREADER_H


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
class configReader_Unit_ParseProgramName_Test;

namespace membrain
{

/**
 * @brief An enumeration to regulate how the swap should define when it approaches it's set boundary
 *
 * Can either keep the boundary fixed, extend it automatically as it needs or start an interactive shell to ask the user for advice
 */
enum class swapPolicy
{
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
 * @note Should not be instantiated by the user, this is done in membrainconfig.h and managedMemory.cpp
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
 * @note Comments can be inserted with leading hash
 * @todo Add more comfort: E.g. memory values should be insertable with unit suffix etc
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
        return readSuccessfullyOnce;
    }

private:
    /**
     * @brief Open stream to config file
     *
     * Order is Custom location > Local user space > Global config
     * @return Success
     */
    bool openStream();

    /**
     * @brief Parse config file
     *
     * Look for default block as well as block matching the current binary name (which has priority of course)
     * @return Success
     * @see parseConfigBlock()
     */
    bool parseConfigFile();
    /**
     * @brief Parse an identified configuration block and extract all matching variables
     * @return Success
     * @warning Does not check for unknown variables or typos, every unknown thing is simply ignored
     * @see parseConfigLine
     */
    bool parseConfigBlock();

    /**
     * @brief Extract the current binary's name out of the /proc file system
     * @return The name
     */
    string getApplicationName() const;

    const string globalConfigPath = "/etc/membrain.conf";
    const string localConfigPath = "~/.membrain.conf";
    string customConfigPath = "";

    const regexMatcher regex;

    ifstream stream;

    configuration config;

    bool readSuccessfullyOnce = false;

    //Test classes
    friend class ::configReader_Unit_ParseProgramName_Test;

};

}

#endif // CONFIGREADER_H

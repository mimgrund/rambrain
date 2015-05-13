#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>
#include <fstream>
#include <istream>

#include "common.h"

using namespace std;

//Test classes
class configReader_Unit_ParseProgramName_Test;

namespace membrain
{

struct configuration {

    configuration();

    string memoryManager = "cyclicManagedMemory";
    global_bytesize memory;
    string swap = "managedFileSwap";
    string swapfiles = "membrainswap-%d";
    global_bytesize swapMemory;
};

class configReader
{

public:
    configReader();

    bool readConfig();

    inline void setCustomConfigPath ( const string &path ) {
        customConfigPath = path;
    }

    inline configuration &getConfig() {
        return config;
    }

    inline bool readSuccessfully() {
        return readSuccessfullyOnce;
    }

private:
    bool openStream();

    bool parseConfigFile();
    bool parseConfigBlock();
    string parseConfigLine ( const string &line, const string &key );

    string getApplicationName();

    const string globalConfigPath = "/etc/membrain.conf";
    const string localConfigPath = "~/.membrain.conf";
    string customConfigPath = "";
    const string defaultBlock = "[default]";

    ifstream stream;

    configuration config;

    bool readSuccessfullyOnce = false;

    //Test classes
    friend class ::configReader_Unit_ParseProgramName_Test;

};

}

#endif // CONFIGREADER_H

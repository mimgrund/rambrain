#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>
#include <fstream>
#include <istream>

using namespace std;

//Test classes
class configReader_Unit_ParseProgramName_Test;

namespace membrain
{

struct configuration {
    string memoryManager = "cyclicManagedMemory";
};

class configReader
{

public:
    configReader();

    bool readConfig();

    inline void setCustomConfigPath ( const string &path ) {
        customConfigPath = path;
    }

    inline configuration getConfig() {
        return config;
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

    //Test classes
    friend class ::configReader_Unit_ParseProgramName_Test;

};

}

#endif // CONFIGREADER_H

#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>
#include <fstream>
#include <istream>

using namespace std;

namespace membrain
{

struct configuration {
    string memoryManager;
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

    string getApplicationName();

    const string globalConfigPath = "/etc/membrain.conf";
    const string localConfigPath = "~/.membrain.conf";
    string customConfigPath = "";
    const string defaultBlock = "[default]";

    ifstream stream;

    configuration config;

};

}

#endif // CONFIGREADER_H

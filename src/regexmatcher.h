#ifndef REGEXMATCHER_H
#define REGEXMATCHER_H

#include <regex>
#include <string>

using namespace std;

namespace membrain
{

/**
 * @brief Class to handle regex matching used for parsing configuration files
 */
class regexMatcher
{

public:
    /**
     * @brief Create a new regex handler, basically a dummy
     */
    regexMatcher();

    /**
     * @brief Checks if a string matches the header of a configuration block
     * @param str The source string
     * @param blockname The config block name to check for
     * @return Result of matching
     */
    bool matchConfigBlock ( const string &str, const string &blockname = "default" ) const;

};

}

#endif // REGEXMATCHER_H

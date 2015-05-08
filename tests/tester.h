#ifndef TESTER_H
#define TESTER_H

#include <vector>
#include <chrono>
#include <fstream>
#include <ostream>
#include <sstream>

class tester
{

public:
    tester ( const char *name );
    virtual ~tester();

    void addParameter ( char *param );
    void addTimeMeasurement();

    void writeToFile();

private:
    const char *name;
    std::vector<char *> parameters;
    std::vector<std::chrono::high_resolution_clock::time_point> timeMeasures;

};

#endif // TESTER_H

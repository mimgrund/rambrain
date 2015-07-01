#ifndef TESTER_H
#define TESTER_H

#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

class tester
{

public:
    tester ( const char *name = "" );
    virtual ~tester();

    void addParameter ( char *param );
    void addTimeMeasurement();
    void addExternalTime ( std::chrono::duration<double> );
    void addComment ( const char *comment );
    void setSeed ( unsigned int seed = time ( NULL ) );

    int random ( int max ) const;
    uint64_t random ( uint64_t max ) const;
    double random ( double max = 1.0 ) const;

    void startNewTimeCycle();
    void startNewRNGCycle();

    void writeToFile();

    std::vector<int64_t> getDurationsForCurrentCycle() const;

private:
    const char *name;
    std::vector<char *> parameters;
    std::vector<std::vector<std::chrono::high_resolution_clock::time_point> > timeMeasures;
    std::string comment;
    std::vector<unsigned int> seeds;
    std::vector<bool> seeded;

};

#endif // TESTER_H

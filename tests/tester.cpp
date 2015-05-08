#include "tester.h"

tester::tester ( const char *name ) : name ( name )
{
}

tester::~tester()
{
}

void tester::addParameter ( char *param )
{
    parameters.push_back ( param );
}

void tester::addTimeMeasurement()
{
    std::chrono::high_resolution_clock::time_point t = std::chrono::high_resolution_clock::now();
    timeMeasures.push_back ( t );
}

void tester::writeToFile()
{
    std::stringstream fileName;
    fileName << ".perftest_" << name;
    for ( auto it = parameters.begin(); it != parameters.end(); ++it ) {
        fileName << "#" << *it;
    }
    std::ofstream out ( fileName.str(), std::ofstream::out );

    out << "#" << name;
    for ( auto it = parameters.begin(); it != parameters.end(); ++it ) {
        out << " " << *it;
    }
    out << std::endl;

    int64_t totms = std::chrono::duration_cast<std::chrono::milliseconds> ( ( *timeMeasures.rbegin() ) - ( *timeMeasures.begin() ) ).count();
    for ( auto it = timeMeasures.begin(), jt = timeMeasures.begin() + 1; it != timeMeasures.end() && jt != timeMeasures.end(); ++it, ++jt ) {
        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds> ( ( *jt ) - ( *it ) ).count();
        out << ms << " " << 100.0 * ms / totms << std::endl;
    }

    out << std::flush;
}

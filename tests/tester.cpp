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
    timeMeasures.back().push_back ( t );
}

void tester::addComment ( const char *comment )
{
    this->comment = std::string ( comment );
}

void tester::startNewCycle()
{
    timeMeasures.push_back ( std::vector<std::chrono::high_resolution_clock::time_point>() );
}

void tester::writeToFile()
{
    std::stringstream fileName;
    fileName << "perftest_" << name;
    for ( auto it = parameters.begin(); it != parameters.end(); ++it ) {
        fileName << "#" << *it;
    }
    std::ofstream out ( fileName.str(), std::ofstream::out );

    out << "# " << name;
    for ( auto it = parameters.begin(); it != parameters.end(); ++it ) {
        out << " " << *it;
    }
    out << std::endl;

    const int cyclesCount = timeMeasures.size();
    out << "# " << cyclesCount << " cycles run for average" << std::endl;

    if ( ! comment.empty() ) {
        out << "# " << comment << std::endl;
    }

    const int timesCount = timeMeasures.front().size() - 1;
    int64_t times[timesCount][cyclesCount];
    double percentages[timesCount][cyclesCount];

    int cycle = 0, time;
    for ( auto repIt = timeMeasures.begin(); repIt != timeMeasures.end(); ++repIt, ++cycle ) {
        int64_t totms = std::chrono::duration_cast<std::chrono::milliseconds> ( repIt->back() - repIt->front() ).count();

        time = 0;
        for ( auto it = repIt->begin(), jt = repIt->begin() + 1; it != repIt->end() && jt != repIt->end(); ++it, ++jt, ++time ) {
            int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds> ( ( *jt ) - ( *it ) ).count();
            times[time][cycle] = ms;
            percentages[time][cycle] = 100.0 * ms / totms;
        }
    }

    int64_t avgTime;
    double avgPercentage;
    for ( time = 0; time < timesCount; ++time ) {

        avgTime = 0;
        avgPercentage = 0.0;

        for ( cycle = 0; cycle < cyclesCount; ++cycle ) {
            avgTime += times[time][cycle];
            avgPercentage += percentages[time][cycle];

            out << times[time][cycle] << "\t" << percentages[time][cycle] << "\t";
        }
        avgTime /= cyclesCount;
        avgPercentage /= cyclesCount;

        out << avgTime << "\t" << avgPercentage << std::endl;
    }

    out << std::flush;
}

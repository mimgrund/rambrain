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

#ifndef PERFORMANCETESTCLASSES_H
#define PERFORMANCETESTCLASSES_H

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>
#include <cstdlib>
#include <map>
#include <inttypes.h>

#include "tester.h"

#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "rambrainconfig.h"

using namespace std;
using namespace rambrain;

/**
 * @brief A base class to encapsulate parameters for performance tests
 */
class testParameterBase
{
public:
    /**
     * @brief Cleanup
     */
    virtual ~testParameterBase() {}

    /**
     * @brief Cast the encapsulated parameter to a string and return it's mean value
     */
    virtual string valueAsString() = 0;
    /**
     * @brief Cast the encapsulated parameter to a string and return it's value at a certain step
     * @param step The step to take
     * @return min + (max - min) * step
     */
    virtual string valueAsString ( unsigned int step ) = 0;

    unsigned int steps;
    bool deltaLog;
    string name;
    bool enabled = true;
};


/**
 * @brief Class to encapsulate parameters given by type T
 */
template<typename T>
class testParameter : public testParameterBase
{

public:
    /**
     * @brief Cleanup
     */
    virtual ~testParameter() {}

    /**
     * @brief Cast the encapsulated parameter to a string and return it's mean value
     */
    virtual inline string valueAsString() {
        return toString ( mean );
    }

    /**
     * @brief Cast the encapsulated parameter to a string and return it's value at a certain step
     * @param step The step to take
     * @return min + (max - min) * step
     */
    virtual inline string valueAsString ( unsigned int step ) {
        return toString ( valueAtStep ( step ) );
    }

    T min, max, mean;

protected:
    /**
     * @brief Look up the parameter value at a certain step
     * @param step The step
     * @return min + (max - min) * step
     */
    T valueAtStep ( unsigned int step ) {
        if ( deltaLog ) {
            return pow ( 10.0, ( log10 ( max ) - log10 ( min ) ) * step / ( steps - 1 ) ) * min;
        } else {
            return ( max - min ) * step / ( steps - 1 ) + min;
        }
    }

    /**
     * @brief Convert a value of the parameter type to a string
     * @param t The value
     * @return The string
     * @note If this fails to compile overloading of the stringstream operator<< is required
     */
    string toString ( const T &t ) {
        stringstream ss;
        ss << t;
        return ss.str();
    }

};


/**
 * @brief Derived performance test classes which take parameter types as template arguments
 */
template<typename... U>
class performanceTest;


/**
 * @brief Base class for all performance tests which itself does not contain any parameters
 */
template<>
class performanceTest<>
{

public:
    /**
     * @brief Create a new performance test
     * @param name The test's name
     */
    performanceTest ( const char *name );
    /**
     * @brief Cleanup
     */
    virtual ~performanceTest() {}

    /**
     * @brief Run this performance test with all given parameter variations and handle data collection and plotting
     * @param repetitions The amount of repetitions for each test
     * @param path Where the performancetests binary is
     */
    virtual void runTests ( unsigned int repetitions, const string &path = "./" );
    /**
     * @brief Run all registered (and enabled) performance tests with all given parameter variations for each test
     * @param repetitions The amount of repetitions for each test
     * @param path Where the performancetests binary is
     */
    static void runRegisteredTests ( unsigned int repetitions, const string &path = "./" );
    /**
     * @brief Enable/Disable a specific test case
     * @param name It's name
     * @param enabled Enable or disable
     */
    static void enableTest ( const string &name, bool enabled );
    /**
     * @brief Enable/Disable all known test cases
     * @param enabled Enable or disable
     */
    static void enableAllTests ( bool enabled );
    /**
     * @brief Completely remove a test case from the setup
     * @param name It's name
     */
    static void unregisterTest ( const string &name );
    /**
     * @brief Write infos about all registered test cases to stdout
     */
    static void dumpTestInfo();

    /**
     * @brief Run a specific test case several times
     * @param name It's name
     * @param myTester The tester class for timing collection
     * @param repetitions How often to repeat the test
     * @param arguments The parameters to pass to the test
     * @param offset At which index the parameters start in the arguments array; Will be incremented to point behind the parameters
     * @param argumentscount How many parameters
     * @return Success
     */
    static bool runRespectiveTest ( const string &name, tester &myTester, unsigned int repetitions, char **arguments, int &offset, int argumentscount );

    /**
     * @brief Contains the actual test code
     * @param test The tester class for timing collection
     * @param arguments The parameters to pass to the test
     * @param offset At which index the parameters start in the arguments array; Will be incremented to point behind the parameters
     * @param argumentscount How many parameters
     */
    virtual void actualTestMethod ( tester &test, char **arguments, int &offset, unsigned int argumentscount ) = 0;

    /**
     * @brief Simple getter
     */
    virtual string getComment() = 0;

    /**
     * @brief If the generated plots shall be immediately displayed
     * @param display Display on or off
     * @note Default is false
     */
    inline static void setDisplayPlots ( bool display ) {
        displayPlots = display;
    }

protected:
    /**
     * @brief Get the amount of steps for parameter variation
     * @param varryParam Which parameter is asked
     * @return The steps
     */
    virtual inline unsigned int getStepsForParam ( unsigned int varryParam ) {
        return parameters[varryParam]->steps;
    }

    /**
     * @brief Create a string using the current parameter values
     * @param varryParam Which parameter is stepped through
     * @param step Which step it is at the moment
     * @param delimiter A delimiter between the parts
     * @return The string
     */
    virtual string getParamsString ( int varryParam, unsigned int step, const string &delimiter = " " );
    /**
     * @brief Creates a string for an outfilename using the current parameter values
     * @param varryParam Which parameter is stepped through
     * @param step Which step it is at the moment
     * @return The string
     */
    virtual string getTestOutfile ( int varryParam, unsigned int step );
    /**
     * @brief Read in average timing results from a test and write the important data to a file
     * @param varryParam Which parameter is stepped through
     * @param step Which step it is at the moment
     * @param file The outfile
     */
    virtual void resultToTempFile ( int varryParam, unsigned int step, ofstream &file );
    /**
     * @brief Split a string into a list of substrings
     * @param in The string
     * @param delimiter Which delimiter to use for splitting
     * @return The list
     */
    virtual vector<string> splitString ( const string &in, char delimiter );
    /**
     * @brief Generate a gnuplot script for swapstats output plotting
     * @param infile Name of the file where the data is stored
     * @param name Name of the output file
     * @param xlabel Label of the x axis
     * @param ylabel Label of the y axis
     * @param title Title of the plot
     * @param log If plot shall be in logscale
     * @param paramColumn Column of the temp file to plot
     * @return The gnuplot script
     * @see resultToTempFile
     */
    virtual string generateGnuplotScript ( const string &infile, const string &name, const string &xlabel, const string &ylabel, const string &title, bool log, int paramColumn );
    /**
     * @brief Handles the timing informations and produces a plot
     * @param varryParam Which parameter is stepped through
     * @param step Which step it is at the moment
     * @param repetitions The amount of repetitions
     */
    virtual void handleTimingInfos ( int varryParam, unsigned int step, unsigned int repetitions );
    /**
     * @brief Extract timing information from a file
     * @param in The file
     * @param start Start time
     * @param end End time
     * @return The relevant data
     */
    virtual vector<vector<string>> getRelevantTimingParts ( ifstream &in, unsigned long long start, unsigned long long end );
    /**
     * @brief Writes timing information to a file
     * @param out The file
     * @param relevantTimingParts The relevant timing data
     * @param starttime Start time; Will be changed to first time in data if zero beforehand
     */
    virtual void timingInfosToFile ( ofstream &out, const vector<vector<string>> &relevantTimingParts, unsigned long long &starttime );
    /**
     * @brief Plot timing information
     * @param gnutemp Outfile for gnuplot script
     * @param outname Name of resulting file
     * @param dataFile Name of data file
     * @param measurements Amount of measurements
     * @param repetitions Amount of repetitions
     * @param linesPoints If points shall be added on top of lines for data points
     */
    virtual void plotTimingInfos ( ofstream &gnutemp, const string &outname, const string &dataFile, unsigned int measurements, unsigned int repetitions, bool linesPoints );
    /**
     * @brief Plot timed hit/miss information
     * @param gnutemp Outfile for gnuplot script
     * @param outname Name of resulting file
     * @param dataFile Name of data file
     * @param measurements Amount of measurements
     * @param repetitions Amount of repetitions
     * @param linesPoints If points shall be added on top of lines for data points
     */
    virtual void plotTimingHitMissInfos ( ofstream &gnutemp, const string &outname, const string &dataFile, unsigned int measurements, unsigned int repetitions, bool linesPoints );

    /**
     * @brief Create a gnuplot script for plotting test results
     * @param file Infile name
     * @param paramColumn Which column to plot as x axis
     * @return The script
     */
    virtual string generateMyGnuplotPlotPart ( const string &file, int paramColumn ) = 0;

    const char *name;
    bool enabled = true;
    vector<testParameterBase *> parameters;
    static map<string, performanceTest<> *> testClasses;
    vector<string> plotParts;
    static bool displayPlots;
    bool plotTimingStats = true;

};


/**
 * @brief Derived performance test classes which take parameter types as template arguments
 */
template<typename T, typename... U>
class performanceTest<T, U...> : public performanceTest<U...>
{

public:
    /**
     * @brief Create a new test object and register it for usage
     * @param name Name of the test
     */
    performanceTest ( const char *name ) : performanceTest<U...> ( name ) {
        this->parameters.push_back ( &parameter );
    }

    /**
     * @brief Cleanup
     */
    virtual ~performanceTest() {}

protected:
    testParameter<T> parameter;

};


#define PARAMREFS(number, param, params...) testParameter<param> &parameter##number = (performanceTest<param, ##params>::parameter)

#define ONEPARAM(param) PARAMREFS(1, param)
#define TWOPARAMS(param1, param2) PARAMREFS(1, param1, param2); \
                                  PARAMREFS(2, param2)
#define THREEARAMS(param1, param2, param3) PARAMREFS(1, param1, param2, param3); \
                                           PARAMREFS(2, param2, param3); \
                                           PARAMREFS(3, param3)

#define ONECONVERT(param) param p = convert<param>(arguments[offset++]); \
                          actualTestMethod(test, p)
#define TWOCONVERT(param1, param2) param1 p1 = convert<param1>(arguments[offset++]); \
                                   param2 p2 = convert<param2>(arguments[offset++]); \
                                   actualTestMethod(test, p1, p2)
#define THREECONVERT(param1, param2, param3) param1 p1 = convert<param1>(arguments[offset++]); \
                                             param2 p2 = convert<param2>(arguments[offset++]); \
                                             param3 p3 = convert<param3>(arguments[offset++]); \
                                             actualTestMethod(test, p1, p2, p3)

#define TESTCLASS(name, parammacro, convertmacro, params...) \
    class name : public performanceTest<params> \
    { \
    public: \
        name(); \
        virtual ~name() {} \
        virtual inline void actualTestMethod(tester& test, char **arguments, int& offset, unsigned int argumentscount); \
        virtual void actualTestMethod(tester&, params); \
        virtual inline string getComment() { return comment; } \
        parammacro; \
        static string comment; \
    protected: \
        virtual string generateMyGnuplotPlotPart(const string& file, int paramColumn); \
        template<typename T> \
        inline T convert(char* str) { \
            return T(str); \
        } \
    }; \
    template<> \
    inline int name::convert<int>(char* str) { \
        return atoi(str); \
    } \
    void name::actualTestMethod(tester& test, char **arguments, int& offset, unsigned int argumentscount) { \
        if (argumentscount - offset < parameters.size()) { \
            cerr << "Not enough parameters supplied for test!" << endl; \
        } else {\
            convertmacro; \
        } \
    } \
    extern name name##Instance

#define ONEPARAMTEST(name, param) TESTCLASS(name, ONEPARAM(param), ONECONVERT(param), param)
#define TWOPARAMTEST(name, param1, param2) TESTCLASS(name, TWOPARAMS(param1, param2), TWOCONVERT(param1, param2), param1, param2)
#define THREEPARAMTEST(name, param1, param2, param3) TESTCLASS(name, THREEPARAMS(param1, param2, param3), THREECONVERT(param1, param2, param3), param1, param2, param3)


// Actual performance test classes come here

TWOPARAMTEST ( matrixTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverTranspose2Test, int, int );
#ifndef OpenMP_NOT_FOUND
TWOPARAMTEST ( matrixCleverTransposeOpenMPTest, int, int );
#endif
TWOPARAMTEST ( matrixCleverBlockTransposeTest, int, int );
#ifndef OpenMP_NOT_FOUND
TWOPARAMTEST ( matrixCleverBlockTransposeOpenMPTest, int, int );
#endif
TWOPARAMTEST ( matrixMultiplyTest, int, int );
#ifndef OpenMP_NOT_FOUND
TWOPARAMTEST ( matrixMultiplyOpenMPTest, int, int );
#endif
TWOPARAMTEST ( matrixCopyTest, int, int );
#ifndef OpenMP_NOT_FOUND
TWOPARAMTEST ( matrixCopyOpenMPTest, int, int );
#endif
TWOPARAMTEST ( matrixDoubleCopyTest, int, int );
#ifndef OpenMP_NOT_FOUND
TWOPARAMTEST ( matrixDoubleCopyOpenMPTest, int, int );
#endif
TWOPARAMTEST ( measureThroughputTest, int, int );
TWOPARAMTEST ( measurePreemptiveSpeedupTest, int, int );
TWOPARAMTEST ( measureExplicitAsyncSpeedupTest, int, int );
TWOPARAMTEST ( measureConstSpeedupTest, int, int );
#endif // PERFORMANCETESTCLASSES_H


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
#include "membrainconfig.h"

using namespace std;
using namespace membrain;

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


template<typename... U>
class performanceTest;


/**
 * Base class for all performance tests which itself does not contain any parameters
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

protected:
    /**
     * @brief Get the amount of steps for parameter variation
     * @param varryParam Which parameter is asked
     * @return The steps
     */
    virtual inline unsigned int getStepsForParam ( unsigned int varryParam ) {
        return parameters[varryParam]->steps;
    }

    virtual string getParamsString ( int varryParam, unsigned int step, const string &delimiter = " " );
    virtual string getTestOutfile ( int varryParam, unsigned int step );
    virtual void resultToTempFile ( int varryParam, unsigned int step, ofstream &file );
    virtual vector<string> splitString ( const string &in, char delimiter );
    virtual string generateGnuplotScript ( const string &name, const string &xlabel, const string &ylabel, const string &title, bool log, int paramColumn );
    virtual void handleTimingInfos ( int varryParam, unsigned int step, unsigned int repetitions );
    virtual vector<vector<string>> getRelevantTimingParts ( ifstream &in, unsigned long long start, unsigned long long end );
    virtual void timingInfosToFile ( ofstream &out, const vector<vector<string>> &relevantTimingParts, unsigned long long &starttime );
    virtual void plotTimingInfos ( ofstream &gnutemp, const string &outname, const string &dataFile, unsigned int measurements, unsigned int repetitions, bool linesPoints );

    virtual string generateMyGnuplotPlotPart ( const string &file, int paramColumn ) = 0;

    const char *name;
    bool enabled = true;
    vector<testParameterBase *> parameters;
    static map<string, performanceTest<> *> testClasses;

};


template<typename T, typename... U>
class performanceTest<T, U...> : public performanceTest<U...>
{

public:
    performanceTest ( const char *name ) : performanceTest<U...> ( name ) {
        this->parameters.push_back ( &parameter );
    }

    virtual ~performanceTest() {}

protected:
    testParameter<T> parameter;

};


#define PARAMREFS(number, param, params...) testParameter<param> &parameter##number = performanceTest<param, ##params>::parameter

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
TWOPARAMTEST ( matrixCleverTransposeOpenMPTest, int, int );
TWOPARAMTEST ( matrixCleverBlockTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverBlockTransposeOpenMPTest, int, int );
TWOPARAMTEST ( matrixMultiplyTest, int, int );
TWOPARAMTEST ( matrixMultiplyOpenMPTest, int, int );
TWOPARAMTEST ( matrixCopyTest, int, int );
TWOPARAMTEST ( matrixCopyOpenMPTest, int, int );
TWOPARAMTEST ( matrixDoubleCopyTest, int, int );
TWOPARAMTEST ( matrixDoubleCopyOpenMPTest, int, int );
TWOPARAMTEST ( measureThroughputTest, int, int );

#endif // PERFORMANCETESTCLASSES_H

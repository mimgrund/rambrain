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

class testParameterBase
{
public:
    virtual ~testParameterBase() {}

    virtual string valueAsString() = 0;
    virtual string valueAsString ( unsigned int step ) = 0;

    unsigned int steps;
    bool deltaLog;
    string name;
    bool enabled = true;
};


template<typename T>
class testParameter : public testParameterBase
{

public:
    virtual ~testParameter() {}

    virtual inline string valueAsString() {
        return toString ( mean );
    }

    virtual inline string valueAsString ( unsigned int step ) {
        return toString ( valueAtStep ( step ) );
    }

    T min, max, mean;

protected:
    T valueAtStep ( unsigned int step ) {
        if ( deltaLog ) {
            return pow ( 10.0, ( log10 ( max ) - log10 ( min ) ) * step / ( steps - 1 ) ) * min;
        } else {
            return ( max - min ) * step / ( steps - 1 ) + min;
        }
    }

    string toString ( const T &t ) {
        stringstream ss;
        ss << t;
        return ss.str();
    }

};


template<typename... U>
class performanceTest;


template<>
class performanceTest<>
{

public:
    performanceTest ( const char *name );
    virtual ~performanceTest() {}

    virtual void runTests ( unsigned int repetitions, const string &path = "./" );
    static void runRegisteredTests ( unsigned int repetitions, const string &path = "./" );
    static void enableTest ( const string &name, bool enabled );
    static void enableAllTests ( bool enabled );
    static void unregisterTest ( const string &name );
    static void dumpTestInfo();

    static bool runRespectiveTest ( const string &name, tester &myTester, unsigned int repetitions, char **arguments, int &offset, int argumentscount );

    virtual void actualTestMethod ( tester &test, char **arguments, int &offset, unsigned int argumentscount ) = 0;
    virtual string getComment() = 0;

protected:
    virtual inline unsigned int getStepsForParam ( unsigned int varryParam ) {
        return parameters[varryParam]->steps;
    }

    virtual string getParamsString ( int varryParam, unsigned int step, const string &delimiter = " " );
    virtual string getTestOutfile ( int varryParam, unsigned int step );
    virtual void resultToTempFile ( int varryParam, unsigned int step, ofstream &file );
    virtual string generateGnuplotScript ( const string &name, const string &xlabel, const string &ylabel, const string &title, bool log, int paramColumn );
    virtual void handleTimingInfos ( int varryParam, unsigned int step, unsigned int repetitions );

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
TWOPARAMTEST ( matrixCleverBlockTransposeOpenMPTest, int, int );
TWOPARAMTEST ( matrixMultiplyTest, int, int );
TWOPARAMTEST ( matrixMultiplyOpenMPTest, int, int );
TWOPARAMTEST ( matrixCopyTest, int, int );
TWOPARAMTEST ( matrixCopyOpenMPTest, int, int );
TWOPARAMTEST ( matrixDoubleCopyTest, int, int );
TWOPARAMTEST ( matrixDoubleCopyOpenMPTest, int, int );

#endif // PERFORMANCETESTCLASSES_H

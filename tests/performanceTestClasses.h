#ifndef PERFORMANCETESTCLASSES_H
#define PERFORMANCETESTCLASSES_H

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>
#include <cstdlib>

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
            return pow ( 10.0, ( log10 ( max ) - log10 ( min ) ) * step / steps );
        } else {
            return ( max - min ) * step / steps;
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


template<typename T>
class performanceTest<T>
{

public:
    performanceTest ( const char *name ) : name ( name ) {
        parameters.push_back ( &parameter );
    }
    virtual ~performanceTest() {}

    virtual bool itsMe ( const string &name ) const {
        return this->name == name;
    }

    virtual void runTestsIfMe ( const string &name, unsigned int repetitions ) {
        if ( itsMe ( name ) ) {
            runTests ( repetitions );
        }
    }

    virtual void runTests ( unsigned int repetitions ) {
        cout << "Running test case " << name << std::endl;
        for ( unsigned int param = 0; param < parameters.size(); ++param ) {
            unsigned int steps = getStepsForParam ( param );
            for ( unsigned int step = 0; step < steps; ++step ) {
                string params = getParamsString ( param, step );
                stringstream call;
                call << "./membrain-performancetests " << repetitions << " " << name << " " << params;
                system ( call.str().c_str() );
            }
            //! @todo do plotting
        }
    }

protected:
    virtual inline unsigned int getStepsForParam ( unsigned int varryParam ) {
        return parameters[parameters.size() - varryParam - 1]->steps;
    }

    virtual string getParamsString ( int varryParam, unsigned int step ) {
        stringstream ss;
        for ( int i = parameters.size() - 1; i >= 0; --i ) {
            if ( i == varryParam ) {
                ss << parameters[i]->valueAsString ( step );
            } else {
                ss << parameters[i]->valueAsString();
            }
            ss << " ";
        }
        return ss.str();
    }

    const char *name;

    testParameter<T> parameter;
    vector<testParameterBase *> parameters;

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
                                           PARAMREFS(2, param2, param3) \
                                           PARAMREFS(3, param3)

#define TESTCLASS(name, parammacro, params...) \
    class name : public performanceTest<params> \
    { \
    public: \
        name(); \
        virtual ~name() {} \
        static void actualTestMethod(tester&, params); \
        parammacro; \
        static string comment; \
    }; \
    extern name name##Instance

#define ONEPARAMTEST(name, param) TESTCLASS(name, ONEPARAM(param), param)
#define TWOPARAMTEST(name, param1, param2) TESTCLASS(name, TWOPARAMS(param1, param2), param1, param2)
#define THREEPARAMTEST(name, param1, param2, param3) TESTCLASS(name, THREEPARAMS(param1, param2, param3), param1, param2, param3)


// Actual performance test classes come here

TWOPARAMTEST ( matrixTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverTransposeTest, int, int );
TWOPARAMTEST ( matrixCleverTransposeOpenMPTest, int, int );
TWOPARAMTEST ( matrixCleverBlockTransposeOpenMPTest, int, int );

#endif // PERFORMANCETESTCLASSES_H

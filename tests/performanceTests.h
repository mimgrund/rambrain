#ifndef PERFORMANCETESTS_H
#define PERFORMANCETESTS_H

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>

#include "tester.h"

#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "membrainconfig.h"

using namespace std;
using namespace membrain;


void runMatrixTranspose ( tester *test, char **args );
void runMatrixCleverTranspose ( tester *test, char **args );
void runMatrixCleverTransposeOpenMP ( tester *test, char **args );
void runMatrixCleverBlockTransposeOpenMP ( tester *test, char **args );



struct testMethod {
    testMethod ( const char *name, int argumentCount, void ( *test ) ( tester *, char ** ), const char *comment )
        : name ( name ), comment ( comment ), argumentCount ( argumentCount ), test ( test ) {}

    const char *name, *comment;
    int argumentCount;
    void ( *test ) ( tester *, char ** );
};



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


//! @todo add plotting
template<typename T>
class performanceTest<T>
{

public:
    performanceTest ( const char *name ) : name ( name ) {
        parameters.push_back ( &parameter );
    }
    virtual ~performanceTest() {}

    virtual void runTests () {
        for ( unsigned int param = 0; param < parameters.size(); ++param ) {
            unsigned int steps = getStepsForParam ( param );
            for ( unsigned int step = 0; step < steps; ++step ) {
                string params = getParamsString ( param, step );
                //! @todo call performance test process
            }
        }
    }

protected:
    inline unsigned int getStepsForParam ( unsigned int varryParam ) {
        return parameters[parameters.size() - varryParam - 1]->steps;
    }

    virtual string getParamsString ( unsigned int varryParam, unsigned int step ) {
        stringstream ss;
        for ( unsigned int i = parameters.size() - 1; i >= 0; --i ) {
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

#define TESTCLASS(name, parammacro, params...) \
    class name : public performanceTest<params> \
    { \
    public: \
        name(); \
        virtual ~name() {} \
        parammacro; \
    protected: \
        static void actualTestMethod(tester&, params); \
    }

#define ONEPARAMTEST(name, param) TESTCLASS(name, ONEPARAM(param), param)
#define TWOPARAMTEST(name, param1, param2) TESTCLASS(name, TWOPARAMS(param1, param2), param1, param2)


// Actual performance test classes come here

TWOPARAMTEST ( matrixTransposeTest, int, int );


#endif // PERFORMANCETESTS_H

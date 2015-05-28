#ifndef PERFORMANCETESTS_H
#define PERFORMANCETESTS_H

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

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




template<typename T>
class testParameter
{

public:
    testParameter() {}
    virtual ~testParameter() {}

    T min, max, mean;
    unsigned int steps;
    bool deltaLog;

};


template<typename... U>
class performanceTest;


template<typename T>
class performanceTest<T>
{

public:
    performanceTest ( const char *name ) : name ( name ) {}
    virtual ~performanceTest() {}

    virtual void runTests () {
        for ( unsigned int i = 0; i < paramCount; ++i ) {
            runTestForParameter ( i );
        }
    }

protected:
    //! @todo add plotting
    void runTestForParameter ( unsigned int i ) {
        //! @todo implement
    }

    virtual void actualTestMethod() = 0;

    virtual void setParameterCount ( unsigned int count ) {
        if ( count > paramCount ) {
            paramCount = count;
        }
    }

    const char *name;

    testParameter<T> parameter;
    unsigned int paramCount = 1;

};


template<typename T, typename... U>
class performanceTest<T, U...> : public performanceTest<U...>
{

public:
    performanceTest ( const char *name ) : performanceTest<U...> ( name ) {
        unsigned int s = 1 + sizeof... ( U );
        if ( s > this->paramCount ) {
            this->paramCount = s;
        }
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
    protected: \
        virtual void actualTestMethod(); \
        parammacro; \
    }

#define ONEPARAMTEST(name, param) TESTCLASS(name, ONEPARAM(param), param)
#define TWOPARAMTEST(name, param1, param2) TESTCLASS(name, TWOPARAMS(param1, param2), param1, param2)


// Actual performance test classes come here

TWOPARAMTEST ( matrixTransposeTest, int, int );


#endif // PERFORMANCETESTS_H

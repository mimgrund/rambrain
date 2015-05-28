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


template<typename T, typename... U>
class performanceTest<T, U...> : public performanceTest<U...>
{

public:
    performanceTest ( const char *name ) : performanceTest<U...> ( name ) {}
    virtual ~performanceTest() {}

protected:
    testParameter<T> parameter;

};


template<typename T>
class performanceTest<T>
{

public:
    performanceTest ( const char *name ) : name ( name ) {}
    virtual ~performanceTest() {}

    virtual void runTests ( const char *comment = "" ) = 0;

protected:
    template <typename U>
    void runTestForParameter ( testParameter<U> parameter ) {
        //! @todo implement
    }

    virtual void actualTestMethod() = 0;

    const char *name;

    testParameter<T> parameter;

};


class matrixTransposeTest : public performanceTest<int, int>
{

public:
    matrixTransposeTest();
    virtual ~matrixTransposeTest() {}

    //! @todo macro for something like this or even to build the whole class declaration?
    //! @todo add plotting
    virtual void runTests ( const char *comment = "" ) {
        runTestForParameter ( firstParameter );
        runTestForParameter ( secondParameter );
    }

protected:
    //! @todo pass parameters, need to adjust method signature based on inheritence signature
    virtual void actualTestMethod();

    //! @todo macro for something like this or even to build the whole class declaration?
    testParameter<int> &firstParameter = performanceTest<int, int>::parameter;
    testParameter<int> &secondParameter = performanceTest<int>::parameter;

};


#endif // PERFORMANCETESTS_H

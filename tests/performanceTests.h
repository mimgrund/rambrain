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


class performanceTest
{

public:
    performanceTest ( const char *name ) : name ( name ) {}
    virtual ~performanceTest() {}

    virtual void runTests ( const char *comment = "" ) = 0;

protected:
    const char *name;

};


template<typename... U>
class parameterCollection;


template<typename T, typename... U>
class parameterCollection<T, U...> : public parameterCollection<U...>
{

public:
    parameterCollection () : parameterCollection<U...>() {}
    virtual ~parameterCollection() {}

protected:
    testParameter<T> parameter;

};


template<typename T>
class parameterCollection<T>
{

public:
    parameterCollection () {}
    virtual ~parameterCollection() {}

protected:
    testParameter<T> parameter;

};


class matrixTransposeTest : public performanceTest, public parameterCollection<int, int>
{

public:
    matrixTransposeTest();
    virtual ~matrixTransposeTest() {}

    virtual void runTests ( const char *comment = "" );

protected:
    //! @todo macro for something like this or even to build the whole class declaration?
    testParameter<int> &firstParameter = parameterCollection<int, int>::parameter;
    testParameter<int> &secondParameter = parameterCollection<int>::parameter;

};


#endif // PERFORMANCETESTS_H

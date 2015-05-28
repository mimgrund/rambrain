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



class testParameter
{

public:
    testParameter() {}
    virtual ~testParameter() {}

};


template<typename T>
class testParameterImpl : testParameter
{

public:
    testParameterImpl() : testParameter() {}
    virtual ~testParameterImpl() {}

    T min, max, delta, mean;

};


template<int parameterCount>
class performanceTest
{

public:
    performanceTest ( const char *name ) : name ( name ) {}
    virtual ~performanceTest() {}

    virtual void runTests ( const char *comment = "" ) = 0;

protected:
    const char *name;
    testParameter parameter[parameterCount];

};


class matrixTransposeTest : public performanceTest<2>
{

public:
    matrixTransposeTest();
    virtual ~matrixTransposeTest() {}

    virtual void runTests ( const char *comment = "" );

};


#endif // PERFORMANCETESTS_H

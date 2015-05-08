#include <iostream>
#include <vector>
#include <cstring>

#include "tester.h"

#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "managedPtr.h"

using namespace std;

void runMatrixTranspose ( tester *test, char **args );


struct testMethod {
    testMethod ( const char *name, int argumentCount, void ( *test ) ( tester *, char ** ), const char *comment )
        : name ( name ), comment ( comment ), argumentCount ( argumentCount ), test ( test ) {}

    const char *name, *comment;
    int argumentCount;
    void ( *test ) ( tester *, char ** );
};


int main ( int argc, char **argv )
{
    std::cout << "Starting performance tests" << std::endl;

    vector<testMethod> tests;
    tests.push_back ( testMethod ( "MatrixTranspose", 2, runMatrixTranspose, "Measurements of allocation and definition, transposition, deletion times" ) );

    int i = 1;
    if ( i >= argc ) {
        std::cerr << "Not enough arguments supplied, expected number of repetitions followed by test directives, exiting" << std::endl;
        return 2;
    }

    int repetitions = atoi ( argv[i++] );

    while ( i < argc ) {
        std::cout << "Attempting to run " << argv[i] << std::endl;

        for ( vector<testMethod>::iterator it = tests.begin(); it != tests.end(); ++it ) {
            if ( ! strcmp ( it->name, argv[i] ) ) {
                ++i;

                if ( i + it->argumentCount > argc ) {
                    std::cerr << "Not enough arguments supplied for test parameters, exiting" << std::endl;
                    return 1;
                }

                tester myTester ( it->name );
                myTester.addComment ( it->comment );

                char **args = 0;
                if ( it->argumentCount > 0 ) {
                    args = new char *[it->argumentCount];
                    for ( int j = 0; j < it->argumentCount; ++j, ++i ) {
                        args[j] = argv[i];
                        myTester.addParameter ( args[j] );
                        std::cout << "Parameter " << j << ": " << args[j] << std::endl;
                    }
                }

                for ( int r = 0; r < repetitions; ++r ) {
                    myTester.startNewCycle();
                    it->test ( &myTester, args );
                }

                myTester.writeToFile();

                if ( it->argumentCount > 0 ) {
                    delete[] args;
                }
            }
        }
    }

    std::cout << "Performance tests done" << std::endl;
    return 0;
}


void runMatrixTranspose ( tester *test, char **args )
{
    const unsigned int size = atoi ( args[0] );
    const unsigned int memlines = atoi ( args[0] );
    const unsigned int mem = size * sizeof ( double ) *  memlines;
    const unsigned int swapmem = size * size * sizeof ( double ) * 2;

    managedFileSwap swap ( swapmem, "/tmp/membrainswap-%d" );
    cyclicManagedMemory manager ( &swap, mem );

    test->addTimeMeasurement();

    // Allocate and set
    managedPtr<double> *rows[size];
    for ( unsigned int i = 0; i < size; ++i ) {
        rows[i] = new managedPtr<double> ( size );
        adhereTo<double> rowloc ( *rows[i] );
        double *rowdbl =  rowloc;
        for ( unsigned int j = 0; j < size; ++j ) {
            rowdbl[j] = i * size + j;
        }
    }

    test->addTimeMeasurement();

    // Transpose
    for ( unsigned int i = 0; i < size; ++i ) {
        adhereTo<double> rowloc1 ( *rows[i] );
        double *rowdbl1 =  rowloc1;
        for ( unsigned int j = i + 1; j < size; ++j ) {
            adhereTo<double> rowloc2 ( *rows[j] );
            double *rowdbl2 =  rowloc2;

            double buffer = rowdbl1[j];
            rowdbl1[j] = rowdbl2[i];
            rowdbl2[i] = buffer;
        }
    }

    test->addTimeMeasurement();


    // Delete
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }

    test->addTimeMeasurement();
}

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

#include "tester.h"
#include "performanceTestClasses.h"

using namespace std;

int main ( int argc, char **argv )
{
    std::cout << "Starting performance tests" << std::endl;

    int i = 1;
    if ( i >= argc ) {
        std::cerr << "Not enough arguments supplied, expected number of repetitions followed by test directives, exiting" << std::endl;
        return 2;
    }

    int repetitions = atoi ( argv[i++] );

    while ( i < argc ) {
        std::cout << "Attempting to run " << argv[i] << std::endl;
        tester myTester ( argv[i] );
        for ( int j = i + 1; j < argc; ++j ) {
            myTester.addParameter ( argv[j] );
        }

        //! @todo can i perhaps auto generate this via the declaration macros? should name be static?
        for ( int r = 0; r < repetitions; ++r ) {
            myTester.startNewTimeCycle();

            if ( matrixTransposeTestInstance.itsMe ( argv[i] ) ) {
                matrixCleverTransposeTestInstance.actualTestMethod ( myTester, atoi ( argv[++i] ), atoi ( argv[++i] ) );
            } else if ( matrixCleverTransposeTestInstance.itsMe ( argv[i] ) ) {
                matrixCleverTransposeTestInstance.actualTestMethod ( myTester, atoi ( argv[++i] ), atoi ( argv[++i] ) );
            } else if ( matrixCleverTransposeOpenMPTestInstance.itsMe ( argv[i] ) ) {
                matrixCleverTransposeOpenMPTestInstance.actualTestMethod ( myTester, atoi ( argv[++i] ), atoi ( argv[++i] ) );
            } else if ( matrixCleverBlockTransposeOpenMPTestInstance.itsMe ( argv[i] ) ) {
                matrixCleverBlockTransposeOpenMPTestInstance.actualTestMethod ( myTester, atoi ( argv[++i] ), atoi ( argv[++i] ) );
            }
        }

        myTester.writeToFile();

        //! @todo plotting?

        /*for ( vector<testMethod>::iterator it = tests.begin(); it != tests.end(); ++it ) {
            if ( strcmp ( it->name, argv[i] ) == 0 ) {
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
                    myTester.startNewTimeCycle();
                    it->test ( &myTester, args );
                }

                myTester.writeToFile();

                if ( it->argumentCount > 0 ) {
                    delete[] args;
                }
                break;
            }
        }
        */
    }

    std::cout << "Performance tests done" << std::endl;
    return 0;
}

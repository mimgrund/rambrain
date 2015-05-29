#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

#include "tester.h"
#include "performanceTestClasses.h"

using namespace std;

int main ( int argc, char **argv )
{
    cout << "Starting performance tests" << endl;
    cout << "Called with: ";
    for ( int i = 0; i < argc; ++i ) {
        cout << argv[i] << " ";
    }
    cout << endl << endl;

    int i = 1;
    if ( i >= argc ) {
        cerr << "Not enough arguments supplied, expected number of repetitions followed by test directives, exiting" << endl;
        return 2;
    }

    int repetitions = atoi ( argv[i++] );

    while ( i < argc ) {
        cout << "Attempting to run " << argv[i] << endl;
        tester myTester ( argv[i] );
        int j;
        for ( j = i + 1; j < argc; ++j ) {
            myTester.addParameter ( argv[j] );
        }

        //! @todo can i perhaps auto generate this via the declaration macros? should name be static?
        for ( int r = 0, j = i; r < repetitions; ++r ) {
            myTester.startNewTimeCycle();

            if ( matrixTransposeTestInstance.itsMe ( argv[j] ) ) {
                matrixTransposeTestInstance.actualTestMethod ( myTester, atoi ( argv[++j] ), atoi ( argv[++j] ) );
            } else if ( matrixCleverTransposeTestInstance.itsMe ( argv[j] ) ) {
                matrixCleverTransposeTestInstance.actualTestMethod ( myTester, atoi ( argv[++j] ), atoi ( argv[++j] ) );
            } else if ( matrixCleverTransposeOpenMPTestInstance.itsMe ( argv[j] ) ) {
                matrixCleverTransposeOpenMPTestInstance.actualTestMethod ( myTester, atoi ( argv[++j] ), atoi ( argv[++j] ) );
            } else if ( matrixCleverBlockTransposeOpenMPTestInstance.itsMe ( argv[j] ) ) {
                matrixCleverBlockTransposeOpenMPTestInstance.actualTestMethod ( myTester, atoi ( argv[++j] ), atoi ( argv[++j] ) );
            } else {
                cerr << "No registered test case matched, aborting..." << endl;
                return 1;
            }
        }
        i = j + 1;

        myTester.writeToFile();

        //! @todo plotting?

        /*for ( vector<testMethod>::iterator it = tests.begin(); it != tests.end(); ++it ) {
            if ( strcmp ( it->name, argv[i] ) == 0 ) {
                ++i;

                if ( i + it->argumentCount > argc ) {
                    cerr << "Not enough arguments supplied for test parameters, exiting" << endl;
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
                        cout << "Parameter " << j << ": " << args[j] << endl;
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

    cout << "Performance tests done" << endl;
    return 0;
}

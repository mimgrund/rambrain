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

        //! @todo run the respective test

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

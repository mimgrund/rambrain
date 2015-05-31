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
        for ( int r = 0, j = i; r < repetitions; ++r, j = i ) {
            myTester.startNewTimeCycle();

            if ( matrixTransposeTestInstance.itsMe ( argv[j] ) ) {
                int p1 = atoi ( argv[++j] ), p2 = atoi ( argv[++j] );
                matrixTransposeTestInstance.actualTestMethod ( myTester, p1, p2 );
            } else if ( matrixCleverTransposeTestInstance.itsMe ( argv[j] ) ) {
                int p1 = atoi ( argv[++j] ), p2 = atoi ( argv[++j] );
                matrixCleverTransposeTestInstance.actualTestMethod ( myTester, p1, p2 );
            } else if ( matrixCleverTransposeOpenMPTestInstance.itsMe ( argv[j] ) ) {
                int p1 = atoi ( argv[++j] ), p2 = atoi ( argv[++j] );
                matrixCleverTransposeOpenMPTestInstance.actualTestMethod ( myTester, p1, p2 );
            } else if ( matrixCleverBlockTransposeOpenMPTestInstance.itsMe ( argv[j] ) ) {
                int p1 = atoi ( argv[++j] ), p2 = atoi ( argv[++j] );
                matrixCleverBlockTransposeOpenMPTestInstance.actualTestMethod ( myTester, p1, p2 );
            } else {
                cerr << "No registered test case matched, aborting..." << endl;
                return 1;
            }
        }
        i = j + 1;

        myTester.writeToFile();
    }

    cout << "Performance tests done" << endl;
    return 0;
}

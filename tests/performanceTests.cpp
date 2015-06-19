#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

#ifdef SWAPSTATS
#include <signal.h>
#endif

#include "tester.h"
#include "performanceTestClasses.h"

using namespace std;

int main ( int argc, char **argv )
{
#ifdef SWAPSTATS
    struct sigaction sigact;

    sigact.sa_handler = SIG_IGN;
    sigemptyset(&sigact.sa_mask);

    if (sigaction(SIGUSR1, &sigact, NULL) < 0) {
        perror ("sigaction");
    }
#endif

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

        ++i;
        bool success = performanceTest<>::runRespectiveTest ( argv[i - 1], myTester, repetitions, argv, i, argc );

        if ( success ) {
            myTester.writeToFile();
        }
    }

    cout << "Performance tests done" << endl;
    return 0;
}

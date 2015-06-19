#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tester.h"
#include "performanceTestClasses.h"

using namespace std;

/*!
 * Usage:
 * * Zero parameters: Execute all test classes, varry all parameters
 * * One parameter:  Overwrite amount of repetitions
 * * More parameters: Overwrite amount of repetitions followed by a + or - sign and the list of test classes to be run or to leave out.
 */
int main ( int argc, char **argv )
{
    cout << "Wrapper for performance tests called" << endl;

    // Repetitions overwritten by first parameter (optional)
    unsigned int repetitions = 10;
    if ( argc > 1 ) {
        repetitions = atoi ( argv[1] );
    }

    // Specialize which test cases to run
    if ( argc > 2 ) {
        bool enable;
        if ( ! strcmp ( argv[2], "+" ) ) {
            performanceTest<>::enableAllTests ( false );
            enable = true;
        } else if ( ! strcmp ( argv[2], "-" ) ) {
            // All tests are on by default
            enable = false;
        } else {
            cerr << argv[2] << " not supported as second parameter, expected + or -" << endl;
            return 1;
        }

        for ( int i = 3; i < argc; ++i ) {
            performanceTest<>::enableTest ( argv[i], enable );
        }
    }

    // Go to the proper directory
    char exe[1024];
    int ret;
    ret = readlink ( "/proc/self/exe", exe, sizeof ( exe ) - 1 );
    if ( ret != -1 ) {
        exe[ret] = '\0';

        int n = 0;
        string file ( exe );
        for ( auto it = file.crbegin(); it != file.crend(); it++, n++ ) {
            if ( ( *it ) == '/' ) {
                break;
            }
        }
        string path = file.substr ( 0, file.length() - n );
        int success = chdir ( path.c_str() );
        if ( ! success ) {
            cout << "Changed working directory to " << path << endl;
        } else {
            cerr << "Could not chdir to " << path << ". Aborting." << endl;
            return 2;
        }
    } else {
        cerr << "Could not read executable path, aborting" << endl;
        return 4;
    }

    // Check if the performance test output directory exists
    struct stat st = {0};
    if ( stat ( "performancetests", &st ) == -1 ) {
        int success = mkdir ( "performancetests", 0777 );
        if ( ! success ) {
            cout << "Created performancetests subdirectory for outputs" << endl;
        } else {
            cerr << "Could not create subdirectory performancetests, aborting." << endl;
            return 3;
        }
    }

    // Change directory to subdir
    int success = chdir ( "performancetests" );
    if ( ! success ) {
        cout << "Changed working directory to subdirectory performancetests" << endl;
    } else {
        cerr << "Could not chdir to subdirectory performancetests. Aborting." << endl;
        return 2;
    }

    cout << endl;


    // Eventually make changes to test parameters of allocated test classes if desired



    // Run tests
    performanceTest<>::dumpTestInfo();
    performanceTest<>::runRegisteredTests ( repetitions, "../" );

    matrixMultiplyOpenMPTestInstance.parameter1.min = 100;
    matrixMultiplyOpenMPTestInstance.parameter1.max = 1000;
    matrixMultiplyOpenMPTestInstance.parameter1.mean = 1000;
    matrixMultiplyOpenMPTestInstance.parameter1.steps = 2;
    matrixMultiplyOpenMPTestInstance.parameter2.steps = 2;

    cout << endl << endl << "All tests done, exiting..." << endl;
    return 0;
}

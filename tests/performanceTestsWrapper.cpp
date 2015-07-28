/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tester.h"
#include "performanceTestClasses.h"

using namespace std;

/**
 * @brief Provides a wrapper binary to run performance tests and scan through respective parameter spaces
 *
 * Usage:
 * * Zero parameters: Execute all test classes, varry all parameters
 * * One parameter:  Overwrite amount of repetitions or "help" / "list" just to list the available tests and the current config
 * * More parameters: Overwrite amount of repetitions followed by a + or - sign and the list of test classes to be run or to leave out.
 */
int main ( int argc, char **argv )
{
    cout << "Wrapper for performance tests called" << endl;

    // Repetitions overwritten by first parameter (optional)
    unsigned int repetitions = 10;
    if ( argc > 1 ) {
        if ( ! strcmp ( argv[1], "help" ) || ! strcmp ( argv[1], "list" ) ) {
            performanceTest<>::dumpTestInfo();
            return 0;
        } else {
            repetitions = atoi ( argv[1] );
        }
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

    cout << endl << endl << "All tests done, exiting..." << endl;
    return 0;
}


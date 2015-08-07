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

#include "performanceTestClasses.h"
#include <chrono>

#ifndef OpenMP_NOT_FOUND
#include <omp.h>
#endif

map<string, performanceTest<> *> performanceTest<>::testClasses;
bool performanceTest<>::displayPlots = false;

performanceTest<>::performanceTest ( const char *name ) : name ( name )
{
    testClasses[name] = this;
}

void performanceTest<>::runTests ( unsigned int repetitions, const string &path  )
{
    cout << "Running test case " << name << std::endl;
    int dummy = 0;

    for ( int param = parameters.size() - 1; param >= 0; --param ) {
        if ( parameters[param]->enabled ) {
            unsigned int steps = getStepsForParam ( param );
            stringstream outname;
            outname << name << param;

            ofstream temp ( outname.str() );

            for ( unsigned int step = 0; step < steps; ++step ) {
                string params = getParamsString ( param, step );
                stringstream call;
                call << path << "rambrain-performancetests " << repetitions << " " << name << " " << params << " 2> /dev/null";
                cout << "Calling: " << call.str() << endl;
                dummy |= system ( call.str().c_str() );

                resultToTempFile ( param, step, temp );
                temp << endl;
#ifdef LOGSTATS
                handleTimingInfos ( param, step, repetitions );
#endif
            }

            temp.close();
            ofstream gnutemp ( "temp.gnuplot" );
            cout << "Generating output file " << outname.str() << endl;
            gnutemp << generateGnuplotScript ( outname.str(), outname.str(), parameters[param]->name, "Execution time [ms]", name, parameters[param]->deltaLog, parameters.size() - param );
            gnutemp.close();

            cout << "Calling gnuplot and displaying result" << endl;
            dummy |= system ( "gnuplot temp.gnuplot" );
            dummy |= system ( ( "convert -density 300 -resize 1920x " + outname.str() + ".eps -flatten " + outname.str() + ".png" ).c_str() );
            if ( displayPlots ) {
                dummy |= system ( ( "display " + outname.str() + ".png &" ).c_str() );
            }

            if ( !dummy ) {
                cerr << "An error in system calls occured..." << endl;
            }
        }
    }
}

void performanceTest<>::runRegisteredTests ( unsigned int repetitions, const string &path )
{
    for ( auto it = testClasses.begin(); it != testClasses.end(); ++it ) {
        performanceTest<> *test = it->second;
        if ( test->enabled ) {
            test->runTests ( repetitions, path );
        } else {
            cout << "Skipping test " << test->name << " because it is disabled." << endl;
        }
    }
}

void performanceTest<>::enableTest ( const string &name, bool enabled )
{
    auto it = testClasses.find ( name );
    if ( it != testClasses.end() ) {
        performanceTest<> *test = it->second;
        test->enabled = enabled;
    } else {
        cerr << "Test " << name << " not found " << endl;
    }
}

void performanceTest<>::enableAllTests ( bool enabled )
{
    for ( auto it = testClasses.begin(); it != testClasses.end(); ++it ) {
        performanceTest<> *test = it->second;
        test->enabled = enabled;
    }
}

void performanceTest<>::unregisterTest ( const string &name )
{
    auto it = testClasses.find ( name );
    if ( it != testClasses.end() ) {
        testClasses.erase ( it );
    } else {
        cerr << "Test " << name << " not found " << endl;
    }
}

void performanceTest<>::dumpTestInfo()
{
    for ( auto it = testClasses.begin(); it != testClasses.end(); ++it ) {
        performanceTest<> *test = it->second;

        cout << "Test class " << test->name << " is currently " << ( test->enabled ? "enabled" : "disabled" ) << ". ";
        cout << "It has " << test->parameters.size() << " parameters:" << endl;

        for ( auto jt = test->parameters.rbegin(); jt != test->parameters.rend(); ++jt ) {
            testParameterBase *param = *jt;

            if ( param->enabled ) {
                cout << "\tFrom\t" << param->valueAsString ( 0 ) << "\tover\t"  << param->valueAsString() << "\tto\t" << param->valueAsString ( param->steps - 1 ) << "\tin ";
                cout << param->steps << ( param->deltaLog ? " logarithmic" : " linear" ) << " steps" << endl;
            } else {
                cout << "\tParameter variation is currently disabled" << endl;
            }
        }

        cout << endl;
    }
}

bool performanceTest<>::runRespectiveTest ( const string &name, tester &myTester, unsigned int repetitions, char **arguments, int &offset, int argumentscount )
{
    auto it = testClasses.find ( name );
    if ( it != testClasses.end() ) {
        performanceTest<> *test = it->second;
        for ( unsigned int r = 0; r < repetitions; ++r ) {
            cout << "Repetition " << ( r + 1 ) << " out of " << repetitions << "                                       " << '\r';
            cout.flush();
            int myOffset = offset;
            myTester.startNewTimeCycle();
            test->actualTestMethod ( myTester, arguments, myOffset, argumentscount );

            if ( r == repetitions - 1 ) {
                offset = myOffset;
            }
        }
        cout << "                                                                                     " << '\r';
        cout.flush();

        return true;
    } else {
        cerr << "Test " << name << " not found " << endl;

        return false;
    }
}

string performanceTest<>::getParamsString ( int varryParam, unsigned int step, const string &delimiter )
{
    stringstream ss;
    for ( int i = parameters.size() - 1; i >= 0; --i ) {
        if ( i == varryParam ) {
            ss << parameters[i]->valueAsString ( step );
        } else {
            ss << parameters[i]->valueAsString();
        }
        ss << delimiter;
    }
    return ss.str();
}

string performanceTest<>::getTestOutfile ( int varryParam, unsigned int step )
{
    stringstream ss;
    ss << name;
    for ( int i = parameters.size() - 1; i >= 0; --i ) {
        if ( i == varryParam ) {
            ss << "#" << parameters[i]->valueAsString ( step );
        } else {
            ss << "#" << parameters[i]->valueAsString();
        }
    }
    return ss.str();
}

void performanceTest<>::resultToTempFile ( int varryParam, unsigned int step, ofstream &file )
{
    file << getParamsString ( varryParam, step, "\t" );
    ifstream test ( getTestOutfile ( varryParam, step ) );
    string line;
    while ( getline ( test, line ) ) {
        if ( line.find ( '#' ) == string::npos ) {
            vector<string> parts = splitString ( line, '\t' );
            file << parts[parts.size() - 2] << '\t';
        }
    }
}

vector<string> performanceTest<>::splitString ( const string &in, char delimiter )
{
    stringstream ss ( in );
    vector<string> parts;
    string part;
    while ( getline ( ss, part, delimiter ) ) {
        parts.push_back ( part );
    }
    return parts;
}

string performanceTest<>::generateGnuplotScript ( const string &infile, const string &name, const string &xlabel, const string &ylabel, const string &title, bool log, int paramColumn )
{
    stringstream ss;
    ss << "set terminal postscript eps enhanced color 'Helvetica,10'" << endl;
    ss << "set output \"" << name << ".eps\"" << endl;
    ss << "set xlabel \"" << xlabel << "\"" << endl;
    ss << "set ylabel \"" << ylabel << "\"" << endl;
    ss << "set title \"" << title << "\"" << endl;
    if ( log ) {
        ss << "set log xy" << endl;
    } else {
        ss << "set log y" << endl;
    }
    ss << generateMyGnuplotPlotPart ( infile, paramColumn );
    return ss.str();
}

void performanceTest<>::handleTimingInfos ( int varryParam, unsigned int step, unsigned int repetitions )
{
    // Move stats file for permanent storage
    string outFile = getTestOutfile ( varryParam, step );
    string timingFile = outFile + "_stats";
    string hitMissFile = outFile + "_hm";
    string tempFile = outFile + "_timing";
    if ( rename ( "rambrain-swapstats.log", timingFile.c_str() ) ) {
        errmsgf ( "Could not rename swapstats log to %s", timingFile.c_str() );
    }

    ifstream testInFile ( outFile );
    ifstream timingInFile ( timingFile );
    ofstream timingTruncFile ( tempFile );

    int initpos = timingInFile.tellg();
    // Go through test output and get first pair of times
    string testLine;
    int measurements = 0, dataPoints = 0;
    unsigned long long starttimes[repetitions];
    for ( unsigned int r = 0; r < repetitions; ++r ) {
        starttimes[r] = 0LLu;
    }

    while ( getline ( testInFile, testLine ) ) {
        if ( testLine.find ( '#' ) == string::npos ) {
            vector<string> testParts = splitString ( testLine, '\t' );

            ++ measurements;
            const unsigned int runCols = 4;
            char *buf;
            timingInFile.seekg ( initpos );
            for ( unsigned int r = 0; r < repetitions; ++r ) {
                unsigned long long start = strtoull ( testParts[r * runCols + 1].c_str(), &buf, 10 );
                unsigned long long end = strtoull ( testParts[r * runCols + 2].c_str(), &buf, 10 );

                // No go through timing file and look for the matching lines there
                vector<vector<string>> relevantTimingParts = getRelevantTimingParts ( timingInFile, start, end );

                // We have all stats for the current run segment, output this data to the temp file
                timingInfosToFile ( timingTruncFile, relevantTimingParts, starttimes[r] );
                dataPoints += relevantTimingParts.size();
            }
        }
    }

    testInFile.close();
    timingInFile.close();
    timingTruncFile.close();

    // Plot that thing
    ofstream gnutemp1 ( "temp.gnuplot" ), gnutemp2 ( "temp2.gnuplot" );
    cout << "Generating output files " << timingFile << " and " << hitMissFile << endl;

    const int maxDataPoints = 50 * repetitions;
    plotTimingInfos ( gnutemp1, timingFile, tempFile, measurements, repetitions, dataPoints <= maxDataPoints );
    plotTimingHitMissInfos ( gnutemp2, hitMissFile, tempFile, measurements, repetitions, dataPoints <= maxDataPoints );

    gnutemp1.close();
    gnutemp2.close();

    cout << "Calling gnuplot and displaying result" << endl;
    int dummy = 0;
    dummy |= system ( "gnuplot temp.gnuplot" );
    dummy |= system ( ( "convert -density 300 -resize 1920x " + timingFile + ".eps -flatten " + timingFile + ".png" ).c_str() );
    if ( displayPlots ) {
        dummy |= system ( ( "display " + timingFile + ".png &" ).c_str() );
    }

    dummy |= system ( "gnuplot temp2.gnuplot" );
    dummy |= system ( ( "convert -density 300 -resize 1920x " + hitMissFile + ".eps -flatten " + hitMissFile + ".png" ).c_str() );
    if ( displayPlots ) {
        dummy |= system ( ( "display " + hitMissFile + ".png &" ).c_str() );
    }

    if ( !dummy ) {
        cerr << "An error in system calls occured..." << endl;
    }
}

vector<vector<string>> performanceTest<>::getRelevantTimingParts ( ifstream &in, unsigned long long start, unsigned long long end )
{
    vector<vector<string>> relevantTimingParts;
    string timingLine;
    char *buf;

    while ( getline ( in, timingLine ) ) {
        if ( timingLine.find ( '#' ) == string::npos ) {
            vector<string> timingParts = splitString ( timingLine, '\t' );

            unsigned long long current = strtoull ( timingParts[0].c_str(), &buf, 10 );

            if ( current >= start && current <= end ) {
                relevantTimingParts.push_back ( timingParts );
            }
            if ( current > end ) {
                break;
            }
        }
    }
    return relevantTimingParts;
}

void performanceTest<>::timingInfosToFile ( ofstream &out, const vector<vector<string>> &relevantTimingParts, unsigned long long &starttime )
{
    out << "#Time since beginning [ms]\tSwappedOut [B]\tSwappedIn [B]\tMemory Used [B]\tSwap Used [B]\tHit / Miss" << endl;
    if ( relevantTimingParts.size() > 0 ) {
        char *buf;
        if ( starttime == 0LLu ) {
            starttime = strtoull ( relevantTimingParts.front() [0].c_str(), &buf, 10 );
        }
        for ( auto it = relevantTimingParts.begin(); it != relevantTimingParts.end(); ++it ) {
            const unsigned long long relTime = strtoull ( ( *it ) [0].c_str(), &buf, 10 ) - starttime;
            const unsigned long long mbOut = strtoul ( ( *it ) [2].c_str(), &buf, 10 ) / mib;
            const unsigned long long mbIn = strtoul ( ( *it ) [5].c_str(), &buf, 10 ) / mib;
            const string hitmiss = ( *it ) [7];
            const unsigned long long mbUsed = strtoul ( ( *it ) [8].c_str(), &buf, 10 ) / mib;
            const unsigned long long mbSwapped = strtoul ( ( *it ) [10].c_str(), &buf, 10 ) / mib;

            out << relTime << " " << mbOut << " " << mbIn << " " << mbUsed << " " << mbSwapped << " " << hitmiss << endl;
        }
    } else {
        out << 0 << " " << 0 << " " << 0 << " " << 0 << " " << 0 << " " << 0.0 << endl;
    }
    out << endl;
}

void performanceTest<>::plotTimingInfos ( ofstream &gnutemp, const string &outname, const string &dataFile, unsigned int measurements, unsigned int repetitions, bool linesPoints )
{
    gnutemp << "set terminal postscript eps enhanced color 'Helvetica,10'" << endl;
    gnutemp << "set output \"" << outname << ".eps\"" << endl;
    gnutemp << "set xlabel \"Time [ms]\"" << endl;
    gnutemp << "set ylabel \"Swap Movement [MB]\"" << endl;
    gnutemp << "set title \"" << name << "\"" << endl;
    gnutemp << "set key top left" << endl;

    if ( linesPoints ) {
        gnutemp << "set style data linespoints" << endl;
    } else {
        gnutemp << "set style data lines" << endl;
    }

    gnutemp << "plot ";
    int c = 1;
    for ( unsigned int m = 0, s = 2; m < measurements; ++m, ++s, ++c ) {
        int mrep = m * repetitions;
        gnutemp << "'" << dataFile << "' every :::" << mrep << "::" << ( mrep + repetitions - 1 ) << " using 1:2 lt 1";
        if ( linesPoints ) {
            gnutemp << " pt " << s;
        }
        gnutemp << " lc " << c << " title \"" << plotParts[m] << ": Swapped out\", \\" << endl;
    }
    for ( unsigned int m = 0, s = 2; m < measurements; ++m, ++s, ++c ) {
        int mrep = m * repetitions;
        gnutemp << "'" << dataFile << "' every :::" << mrep << "::" << ( mrep + repetitions - 1 ) << " using 1:3 lt 1";
        if ( linesPoints ) {
            gnutemp << " pt " << s;
        }
        gnutemp << " lc " << c << " title \"" << plotParts[m] << ": Swapped in\", \\" << endl;
    }
    for ( unsigned int m = 0, s = 2; m < measurements; ++m, ++s, ++c ) {
        int mrep = m * repetitions;
        gnutemp << "'" << dataFile << "' every :::" << mrep << "::" << ( mrep + repetitions - 1 ) << " using 1:4 lt 2";
        if ( linesPoints ) {
            gnutemp << " pt " << s;
        }
        gnutemp << " lc " << c << " title \"" << plotParts[m] << ": Main Memory\", \\" << endl;
    }
    for ( unsigned int m = 0, s = 2; m < measurements; ++m, ++s, ++c ) {
        int mrep = m * repetitions;
        gnutemp << "'" << dataFile << "' every :::" << mrep << "::" << ( mrep + repetitions - 1 ) << " using 1:5 lt 2";
        if ( linesPoints ) {
            gnutemp << " pt " << s;
        }
        gnutemp << " lc " << c << " title \"" << plotParts[m] << ": Swap Memory\"";
        if ( m != measurements - 1 ) {
            gnutemp << ", \\";
        }
        gnutemp << endl;
    }
}

void performanceTest<>::plotTimingHitMissInfos ( ofstream &gnutemp, const string &outname, const string &dataFile, unsigned int measurements, unsigned int repetitions, bool linesPoints )
{
    gnutemp << "set terminal postscript eps enhanced color 'Helvetica,10'" << endl;
    gnutemp << "set output \"" << outname << ".eps\"" << endl;
    gnutemp << "set xlabel \"Time [ms]\"" << endl;
    gnutemp << "set ylabel \"Hit / Miss ratio\"" << endl;
    gnutemp << "set title \"" << name << "\"" << endl;
    gnutemp << "set log y" << endl;

    if ( linesPoints ) {
        gnutemp << "set style data linespoints" << endl;
    } else {
        gnutemp << "set style data lines" << endl;
    }

    gnutemp << "plot ";
    int c = 1;
    for ( unsigned int m = 0, s = 2; m < measurements; ++m, ++s, ++c ) {
        int mrep = m * repetitions;
        gnutemp << "'" << dataFile << "' every :::" << mrep << "::" << ( mrep + repetitions - 1 ) << " using 1:6 lt 1";
        if ( linesPoints ) {
            gnutemp << " pt " << s;
        }
        gnutemp << " lc " << c << " title \"" << plotParts[m] << ": Hit / Miss\", \\" << endl;
    }
}


//! TEST CLASSES BEGIN HERE, SOME CONVENIENCE MACROS:

#define TESTSTATICS(name, commenttext) string name::comment = commenttext; \
                                       name name##Instance

#define TESTPARAM(param, minimum, maximum, nrsteps, log, meanvalue, paramname) parameter##param.min = minimum; \
                                                                                   parameter##param.max = maximum; \
                                                                                   parameter##param.steps = nrsteps; \
                                                                                   parameter##param.deltaLog = log; \
                                                                                   parameter##param.mean = meanvalue; \
                                                                                   parameter##param.name = paramname



TESTSTATICS ( matrixTransposeTest, "Measurements of allocation and definition, transposition, deletion times" );

matrixTransposeTest::matrixTransposeTest() : performanceTest<int, int> ( "MatrixTranspose" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Transposition", "Deletion"} );
}

void matrixTransposeTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 2;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );

    test.addTimeMeasurement();

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

    test.addTimeMeasurement();

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

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( unsigned int i = 0; i < size; ++i ) {
        adhereTo<double> rowloc ( *rows[i] );
        double *rowdbl =  rowloc;
        for ( unsigned int j = 0; j < size; ++j ) {
            if ( rowdbl[j] != j * size + i ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }

    test.addTimeMeasurement();
}

string matrixTransposeTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"" << endl;
    return ss.str();
}


TESTSTATICS ( matrixCleverTransposeTest, "Measurements of allocation and definition, transposition, deletion times, but with a clever transposition algorithm" );

matrixCleverTransposeTest::matrixCleverTransposeTest() : performanceTest<int, int> ( "MatrixCleverTranspose" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Transposition", "Deletion"} );
}

void matrixCleverTransposeTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 2;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );

    test.addTimeMeasurement();

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

    test.addTimeMeasurement();

    // Transpose blockwise
    unsigned int rows_fetch = memlines / 2 > size ? size : memlines / 2;
    unsigned int n_blocks = size / rows_fetch + ( size % rows_fetch == 0 ? 0 : 1 );

    adhereTo<double> *Arows[rows_fetch];
    adhereTo<double> *Brows[rows_fetch];

    for ( unsigned int jj = 0; jj < n_blocks; jj++ ) {
        for ( unsigned int ii = 0; ii <= jj; ii++ ) {
            //A_iijj <-> B_jjii

            //Reserve rows ii and jj
            unsigned int i_lim = ( ii + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, vertical limit
            unsigned int j_lim = ( jj + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, horizontal limit
            unsigned int i_off = ii * rows_fetch; // Block A, vertical index
            unsigned int j_off = jj * rows_fetch; // Block A, horizontal index

            //Get rows A_ii** and B_jj** into memory:
            for ( unsigned int i = 0; i < i_lim; ++i ) {
                Arows[i] = new adhereTo<double> ( *rows[i + i_off] );
            }
            for ( unsigned int j = 0; j < j_lim; ++j ) {
                Brows[j] = new adhereTo<double> ( *rows[j + j_off] );
            }

            for ( unsigned int j = 0; j < j_lim; j++ ) {
                for ( unsigned int i = 0; i < ( jj == ii ? j : i_lim ); i++ ) { //Inner block matrix transpose, vertical index in A
                    //Inner block matrxi transpose, horizontal index in A
                    double *Arowdb = *Arows[i]; //Fetch pointer for Element of A_ii+i
                    double *Browdb = *Brows[j];

                    double inter = Arowdb[j_off + j]; //Store inner element A_ij
                    Arowdb[j + j_off] = Browdb[ i + i_off]; //Override with element of B_ji
                    Browdb[i + i_off] = inter; //set B_ji to former val of A_ij
                }
            }

            for ( unsigned int i = 0; i < i_lim; ++i ) {
                delete ( Arows[i] );
            }
            for ( unsigned int j = 0; j < j_lim; ++j ) {
                delete ( Brows[j] );
            }
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( unsigned int i = 0; i < size; ++i ) {
        adhereTo<double> rowloc ( *rows[i] );
        double *rowdbl =  rowloc;
        for ( unsigned int j = 0; j < size; ++j ) {
            if ( rowdbl[j] != j * size + i ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }

    test.addTimeMeasurement();
}

string matrixCleverTransposeTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


#ifndef OpenMP_NOT_FOUND
TESTSTATICS ( matrixCleverTransposeOpenMPTest, "Same as cleverTranspose, but with OpenMP" );

matrixCleverTransposeOpenMPTest::matrixCleverTransposeOpenMPTest() : performanceTest<int, int> ( "MatrixCleverTransposeOpenMP" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Transposition", "Deletion"} );
}

void matrixCleverTransposeOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );

    test.addTimeMeasurement();

    // Allocate and set
    managedPtr<double> *rows[size];
    #pragma omp parallel for
    for ( unsigned int i = 0; i < size; ++i ) {
        rows[i] = new managedPtr<double> ( size );
        adhereTo<double> rowloc ( *rows[i] );
        double *rowdbl =  rowloc;
        for ( unsigned int j = 0; j < size; ++j ) {
            rowdbl[j] = i * size + j;
        }
    }

    test.addTimeMeasurement();

    // Transpose blockwise, leave a bit free space, if not, we're stuck in the process...

    unsigned int rows_fetch = memlines / ( 4 ) > size ? size : memlines / ( 4 );
    unsigned int n_blocks = size / rows_fetch + ( size % rows_fetch == 0 ? 0 : 1 );

    adhereTo<double> *Arows[rows_fetch];
    adhereTo<double> *Brows[rows_fetch];

    for ( unsigned int jj = 0; jj < n_blocks; jj++ ) {
        for ( unsigned int ii = 0; ii <= jj; ii++ ) {
            //A_iijj <-> B_jjii

            //Reserve rows ii and jj
            unsigned int i_lim = ( ii + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, vertical limit
            unsigned int j_lim = ( jj + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, horizontal limit
            unsigned int i_off = ii * rows_fetch; // Block A, vertical index
            unsigned int j_off = jj * rows_fetch; // Block A, horizontal index

            //Get rows A_ii** and B_jj** into memory:
            #pragma omp parallel for
            for ( unsigned int i = 0; i < i_lim; ++i ) {
                Arows[i] = new adhereTo<double> ( *rows[i + i_off], true );
            }
            #pragma omp parallel for
            for ( unsigned int j = 0; j < j_lim; ++j ) {
                Brows[j] = new adhereTo<double> ( *rows[j + j_off], true );
            }
            #pragma omp parallel for
            for ( unsigned int j = 0; j < j_lim; j++ ) {
                for ( unsigned int i = 0; i < ( jj == ii ? j : i_lim ); i++ ) { //Inner block matrix transpose, vertical index in A
                    //Inner block matrxi transpose, horizontal index in A
                    double *Arowdb = *Arows[i]; //Fetch pointer for Element of A_ii+i
                    double *Browdb = *Brows[j];

                    double inter = Arowdb[j_off + j]; //Store inner element A_ij
                    Arowdb[j + j_off] = Browdb[ i + i_off]; //Override with element of B_ji
                    Browdb[i + i_off] = inter; //set B_ji to former val of A_ij
                }
            }
            #pragma omp parallel for
            for ( unsigned int i = 0; i < i_lim; ++i ) {
                delete ( Arows[i] );
            }
            #pragma omp parallel for
            for ( unsigned int j = 0; j < j_lim; ++j ) {
                delete ( Brows[j] );
            }
        }
    }


    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( unsigned int i = 0; i < size; ++i ) {
        adhereTo<double> rowloc ( *rows[i] );
        double *rowdbl =  rowloc;
        for ( unsigned int j = 0; j < size; ++j ) {
            if ( rowdbl[j] != j * size + i ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    #pragma omp parallel for
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }
    test.addTimeMeasurement();
}

string matrixCleverTransposeOpenMPTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}
#endif


TESTSTATICS ( matrixCleverBlockTransposeTest, "Same as cleverTranspose, but with blockwise multiplication" );

matrixCleverBlockTransposeTest::matrixCleverBlockTransposeTest() : performanceTest<int, int> ( "MatrixCleverBlockTranspose" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Transposition", "Deletion"} );
}

void matrixCleverBlockTransposeTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Transpose blockwise, leave a bit free space, if not, we're stuck in the process...

    unsigned int rows_fetch = sqrt ( memlines * size / 2 );
    unsigned int blocksize = rows_fetch * rows_fetch;

    unsigned int n_blocks = size / rows_fetch + ( size % rows_fetch == 0 ? 0 : 1 );

    //Blocks are rows_fetch² matrices stored in n_blocks² blocks.

#define blockIdx(x,y) ((x/rows_fetch)*n_blocks+y/rows_fetch)
#define inBlockX(x,y) (x%rows_fetch)
#define inBlockY(x,y) (y%rows_fetch)
#define inBlockIdx(x,y) (inBlockX(x,y)*rows_fetch+inBlockY(x,y))
    // Allocate and set
    managedPtr<double> *rows[n_blocks * n_blocks];
    for ( unsigned int jj = 0; jj < n_blocks; jj++ ) {
        for ( unsigned int ii = 0; ii < n_blocks; ii++ ) {
            rows[ii * n_blocks + jj] = new managedPtr<double> ( blocksize );
            adhereTo<double> adh ( *rows[ii * n_blocks + jj] );
            double *locPtr = adh;
            unsigned int i_lim = ( ii + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, vertical limit
            unsigned int j_lim = rows_fetch;//( jj + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, horizontal limit
            for ( unsigned int i = 0; i < i_lim; i++ ) {
                for ( unsigned int j = 0; j < j_lim; j++ ) {
                    locPtr[i * rows_fetch + j] = ( ii * rows_fetch + i ) * size + ( j + rows_fetch * jj );
                }
            }
        }
    }
    test.addTimeMeasurement();

    for ( unsigned int jj = 0; jj < n_blocks; jj++ ) {
        for ( unsigned int ii = 0; ii <= jj; ii++ ) {
            //A_iijj <-> B_jjii

            //Reserve rows ii and jj
            unsigned int i_lim = ( ii + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, vertical limit
            unsigned int j_lim = ( jj + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, horizontal limit

            //Get rows A_ii** and B_jj** into memory:
            adhereTo<double> aBlock ( *rows[ii * n_blocks + jj] );
            adhereTo<double> bBlock ( *rows[jj * n_blocks + ii] );

            double *aLoc = aBlock;
            double *bLoc = bBlock;

            for ( unsigned int j = 0; j < j_lim; j++ ) {
                for ( unsigned int i = 0; i < ( jj == ii ? j : i_lim ); i++ ) { //Inner block matrix transpose, vertical index in A
                    //Inner block matrxi transpose, horizontal index in A
                    double inter = aLoc[i * rows_fetch + j]; //Store inner element A_ij
                    aLoc[i * rows_fetch + j] = bLoc[j * rows_fetch + i]; //Override with element of B_ji
                    bLoc[j * rows_fetch + i] = inter; //set B_ji to former val of A_ij
                }
            }
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( unsigned int i = 0; i < size; ++i ) {
        for ( unsigned int j = 0; j < size; ++j ) {
            unsigned int blckidx = blockIdx ( i, j );
            unsigned int inblck = inBlockIdx ( i, j );
            adhereTo<double> adh ( *rows[blckidx] );
            double *loc = adh;
            if ( loc[inblck] != j * size + i ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    for ( unsigned int i = 0; i < n_blocks * n_blocks; ++i ) {
        delete rows[i];
    }

    test.addTimeMeasurement();
}

string matrixCleverBlockTransposeTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


#ifndef OpenMP_NOT_FOUND
TESTSTATICS ( matrixCleverBlockTransposeOpenMPTest, "Same as cleverTranspose, but with OpenMP and blockwise multiplication" );

matrixCleverBlockTransposeOpenMPTest::matrixCleverBlockTransposeOpenMPTest() : performanceTest<int, int> ( "MatrixCleverBlockTransposeOpenMP" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Transposition", "Deletion"} );
}

void matrixCleverBlockTransposeOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Transpose blockwise, leave a bit free space, if not, we're stuck in the process...

    unsigned int rows_fetch = sqrt ( memlines * size / 2 );
    unsigned int blocksize = rows_fetch * rows_fetch;

    unsigned int n_blocks = size / rows_fetch + ( size % rows_fetch == 0 ? 0 : 1 );

    //Blocks are rows_fetch² matrices stored in n_blocks² blocks.

#define blockIdx(x,y) ((x/rows_fetch)*n_blocks+y/rows_fetch)
#define inBlockX(x,y) (x%rows_fetch)
#define inBlockY(x,y) (y%rows_fetch)
#define inBlockIdx(x,y) (inBlockX(x,y)*rows_fetch+inBlockY(x,y))
    // Allocate and set
    managedPtr<double> *rows[n_blocks * n_blocks];
    for ( unsigned int jj = 0; jj < n_blocks; jj++ ) {
        for ( unsigned int ii = 0; ii < n_blocks; ii++ ) {
            rows[ii * n_blocks + jj] = new managedPtr<double> ( blocksize );
            adhereTo<double> adh ( *rows[ii * n_blocks + jj] );
            double *locPtr = adh;
            unsigned int i_lim = ( ii + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, vertical limit
            unsigned int j_lim = rows_fetch;//( jj + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, horizontal limit
            #pragma omp parallel for
            for ( unsigned int i = 0; i < i_lim; i++ ) {
                for ( unsigned int j = 0; j < j_lim; j++ ) {
                    locPtr[i * rows_fetch + j] = ( ii * rows_fetch + i ) * size + ( j + rows_fetch * jj );
                }
            }
        }
    }
    test.addTimeMeasurement();

    for ( unsigned int jj = 0; jj < n_blocks; jj++ ) {
        for ( unsigned int ii = 0; ii <= jj; ii++ ) {
            //A_iijj <-> B_jjii

            //Reserve rows ii and jj
            unsigned int i_lim = ( ii + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, vertical limit
            unsigned int j_lim = ( jj + 1 == n_blocks && size % rows_fetch != 0 ? size % rows_fetch : rows_fetch ); // Block A, horizontal limit

            //Get rows A_ii** and B_jj** into memory:
            adhereTo<double> aBlock ( *rows[ii * n_blocks + jj] );
            adhereTo<double> bBlock ( *rows[jj * n_blocks + ii] );

            double *aLoc = aBlock;
            double *bLoc = bBlock;

            #pragma omp parallel for
            for ( unsigned int j = 0; j < j_lim; j++ ) {
                for ( unsigned int i = 0; i < ( jj == ii ? j : i_lim ); i++ ) { //Inner block matrix transpose, vertical index in A
                    //Inner block matrxi transpose, horizontal index in A
                    double inter = aLoc[i * rows_fetch + j]; //Store inner element A_ij
                    aLoc[i * rows_fetch + j] = bLoc[j * rows_fetch + i]; //Override with element of B_ji
                    bLoc[j * rows_fetch + i] = inter; //set B_ji to former val of A_ij
                }
            }
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( unsigned int i = 0; i < size; ++i ) {
        for ( unsigned int j = 0; j < size; ++j ) {
            unsigned int blckidx = blockIdx ( i, j );
            unsigned int inblck = inBlockIdx ( i, j );
            adhereTo<double> adh ( *rows[blckidx] );
            double *loc = adh;
            if ( loc[inblck] != j * size + i ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    #pragma omp parallel for
    for ( unsigned int i = 0; i < n_blocks * n_blocks; ++i ) {
        delete rows[i];
    }

    test.addTimeMeasurement();
}

string matrixCleverBlockTransposeOpenMPTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}
#endif


TESTSTATICS ( matrixMultiplyTest, "Matrix multiplication with matrices being stored in columns / rows" );

matrixMultiplyTest::matrixMultiplyTest() : performanceTest<int, int> ( "MatrixMultiply" )
{
    TESTPARAM ( 1, 10, 6000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 4000, 15000, 20, true, 6000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Multiplication", "Deletion"} );
}

void matrixMultiplyTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Allocate and set matrixes A, B and C
    managedPtr<double> *rowsA[size];
    managedPtr<double> *colsB[size];
    managedPtr<double> *rowsC[size];
    for ( global_bytesize i = 0; i < size; ++i ) {
        rowsA[i] = new managedPtr<double> ( size );
        colsB[i] = new managedPtr<double> ( size );
        rowsC[i] = new managedPtr<double> ( size );

        adhereTo<double> adhRowA ( *rowsA[i] );
        adhereTo<double> adhColB ( *colsB[i] );
        adhereTo<double> adhRowC ( *rowsC[i] );

        double *rowA = adhRowA;
        double *colB = adhColB;
        double *rowC = adhRowC;

        for ( global_bytesize j = 0; j < size; ++j ) {
            rowA[j] = j;
            // B = transpose(A)
            colB[j] = j;
            rowC[j] = 0.0;
        }
    }

    test.addTimeMeasurement();

    // Calculate C = A * B
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhRowA ( *rowsA[i] );
        adhereTo<double> adhRowC ( *rowsC[i] );
        double *rowA = adhRowA;
        double *rowC = adhRowC;
        for ( global_bytesize j = 0; j < size; ++j ) {
            adhereTo<double> adhColB ( *colsB[j] );
            double *colB = adhColB;
            double erg = 0;

            for ( global_bytesize k = 0; k < size; ++k ) {
                erg += rowA[k] * colB[k];
            }
            rowC[j] += erg;
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    double val = 0.0;
    for ( global_bytesize i = 1; i <= size; ++i ) {
        val += i * i;
    }
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhRowC ( *rowsC[i] );

        double *rowC = adhRowC;

        for ( global_bytesize j = 0; j < size; ++j ) {
            if ( rowC[j] != val ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    for ( global_bytesize i = 0; i < size; ++i ) {
        delete rowsA[i];
        delete colsB[i];
        delete rowsC[i];
    }

    test.addTimeMeasurement();
}

string matrixMultiplyTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Multiplication\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


#ifndef OpenMP_NOT_FOUND
TESTSTATICS ( matrixMultiplyOpenMPTest, "Matrix multiplication with matrices being stored in columns / rows" );

matrixMultiplyOpenMPTest::matrixMultiplyOpenMPTest() : performanceTest<int, int> ( "MatrixMultiplyOpenMP" )
{
    TESTPARAM ( 1, 10, 6000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 4000, 15000, 20, true, 6000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Multiplication", "Deletion"} );
}

void matrixMultiplyOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Allocate and set matrixes A, B and C
    managedPtr<double> *rowsA[size];
    managedPtr<double> *colsB[size];
    managedPtr<double> *rowsC[size];

    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        rowsA[i] = new managedPtr<double> ( size );
        colsB[i] = new managedPtr<double> ( size );
        rowsC[i] = new managedPtr<double> ( size );

        adhereTo<double> adhRowA ( *rowsA[i] );
        adhereTo<double> adhColB ( *colsB[i] );
        adhereTo<double> adhRowC ( *rowsC[i] );

        double *rowA = adhRowA;
        double *colB = adhColB;
        double *rowC = adhRowC;

        for ( global_bytesize j = 0; j < size; ++j ) {
            rowA[j] = j;
            colB[j] = j;
            rowC[j] = 0.0;
        }
    }

    test.addTimeMeasurement();

    // Calculate C = A * B
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhRowA ( *rowsA[i] );
        adhereTo<double> adhRowC ( *rowsC[i] );
        double *rowA = adhRowA;
        double *rowC = adhRowC;
        for ( global_bytesize j = 0; j < size; ++j ) {
            adhereTo<double> adhColB ( *colsB[j] );
            double *colB = adhColB;
            double erg = 0;

            for ( global_bytesize k = 0; k < size; ++k ) {
                erg += rowA[k] * colB[k];
            }
            rowC[j] += erg;
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    double val = 0.0;
    for ( global_bytesize i = 1; i <= size; ++i ) {
        val += i * i;
    }
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhRowC ( *rowsC[i] );

        double *rowC = adhRowC;

        for ( global_bytesize j = 0; j < size; ++j ) {
            if ( rowC[j] != val ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        delete rowsA[i];
        delete colsB[i];
        delete rowsC[i];
    }

    test.addTimeMeasurement();
}

string matrixMultiplyOpenMPTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Multiplication\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}
#endif


TESTSTATICS ( matrixCopyTest, "Copy one matrix onto another" );

matrixCopyTest::matrixCopyTest() : performanceTest<int, int> ( "MatrixCopy" )
{
    TESTPARAM ( 1, 100, 10000, 20, true, 5000, "Matrix size per dimension" );
    TESTPARAM ( 2, 100, 10000, 20, true, 5000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Copy", "Deletion"} );
}

void matrixCopyTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Allocate and set matrixes A, B
    managedPtr<double> *A[size];
    managedPtr<double> *B[size];

    for ( global_bytesize i = 0; i < size; ++i ) {
        A[i] = new managedPtr<double> ( size );
        B[i] = new managedPtr<double> ( size );

        adhereTo<double> adhA ( *A[i] );

        double *a = adhA;

        for ( global_bytesize j = 0; j < size; ++j ) {
            a[j] = j;
        }
    }

    test.addTimeMeasurement();

    // Copy B = A
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhA ( *A[i] );
        adhereTo<double> adhB ( *B[i] );

        double *a = adhA;
        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            b[j] = a[j];
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhB ( *B[i] );

        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            if ( b[j] != j ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    for ( global_bytesize i = 0; i < size; ++i ) {
        delete A[i];
        delete B[i];
    }

    test.addTimeMeasurement();
}

string matrixCopyTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Copy\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


#ifndef OpenMP_NOT_FOUND
TESTSTATICS ( matrixCopyOpenMPTest, "Copy one matrix onto another" );

matrixCopyOpenMPTest::matrixCopyOpenMPTest() : performanceTest<int, int> ( "MatrixCopyOpenMP" )
{
    TESTPARAM ( 1, 100, 10000, 20, true, 5000, "Matrix size per dimension" );
    TESTPARAM ( 2, 100, 10000, 20, true, 5000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Copy", "Deletion"} );
}

void matrixCopyOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Allocate and set matrixes A, B
    managedPtr<double> *A[size];
    managedPtr<double> *B[size];

    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        A[i] = new managedPtr<double> ( size );
        B[i] = new managedPtr<double> ( size );

        adhereTo<double> adhA ( *A[i] );

        double *a = adhA;

        for ( global_bytesize j = 0; j < size; ++j ) {
            a[j] = j;
        }
    }

    test.addTimeMeasurement();

    // Copy B = A
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhA ( *A[i] );
        adhereTo<double> adhB ( *B[i] );

        double *a = adhA;
        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            b[j] = a[j];
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhB ( *B[i] );

        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            if ( b[j] != j ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        delete A[i];
        delete B[i];
    }

    test.addTimeMeasurement();
}

string matrixCopyOpenMPTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Copy\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}
#endif


TESTSTATICS ( matrixDoubleCopyTest, "Copy one matrix onto another and back" );

matrixDoubleCopyTest::matrixDoubleCopyTest() : performanceTest<int, int> ( "MatrixDoubleCopy" )
{
    TESTPARAM ( 1, 100, 10000, 20, true, 5000, "Matrix size per dimension" );
    TESTPARAM ( 2, 100, 10000, 20, true, 5000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Copy", "Deletion"} );
}

void matrixDoubleCopyTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Allocate and set matrixes A, B
    managedPtr<double> *A[size];
    managedPtr<double> *B[size];

    for ( global_bytesize i = 0; i < size; ++i ) {
        A[i] = new managedPtr<double> ( size );
        B[i] = new managedPtr<double> ( size );

        adhereTo<double> adhA ( *A[i] );

        double *a = adhA;

        for ( global_bytesize j = 0; j < size; ++j ) {
            a[j] = j;
        }
    }

    test.addTimeMeasurement();

    // Copy B = A
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhA ( *A[i] );
        adhereTo<double> adhB ( *B[i] );

        double *a = adhA;
        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            b[j] = a[j];
        }
    }

    // Copy A = B
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhA ( *A[i] );
        adhereTo<double> adhB ( *B[i] );

        double *a = adhA;
        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            a[j] = b[j];
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhB ( *B[i] );

        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            if ( a[j] != j || b[j] != j ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    for ( global_bytesize i = 0; i < size; ++i ) {
        delete A[i];
        delete B[i];
    }

    test.addTimeMeasurement();
}

string matrixDoubleCopyTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Copy\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


#ifndef OpenMP_NOT_FOUND
TESTSTATICS ( matrixDoubleCopyOpenMPTest, "Copy one matrix onto another and back" );

matrixDoubleCopyOpenMPTest::matrixDoubleCopyOpenMPTest() : performanceTest<int, int> ( "MatrixDoubleCopyOpenMP" )
{
    TESTPARAM ( 1, 100, 10000, 20, true, 5000, "Matrix size per dimension" );
    TESTPARAM ( 2, 100, 10000, 20, true, 5000, "Matrix rows in main memory" );
    plotParts = vector<string> ( {"Allocation \\\\& Definition", "Copy", "Deletion"} );
}

void matrixDoubleCopyOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    rambrainglobals::config.resizeMemory ( mem );
    rambrainglobals::config.resizeSwap ( swapmem );


    test.addTimeMeasurement();

    // Allocate and set matrixes A, B
    managedPtr<double> *A[size];
    managedPtr<double> *B[size];

    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        A[i] = new managedPtr<double> ( size );
        B[i] = new managedPtr<double> ( size );

        adhereTo<double> adhA ( *A[i] );

        double *a = adhA;

        for ( global_bytesize j = 0; j < size; ++j ) {
            a[j] = j;
        }
    }

    test.addTimeMeasurement();

    // Copy B = A
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhA ( *A[i] );
        adhereTo<double> adhB ( *B[i] );

        double *a = adhA;
        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            b[j] = a[j];
        }
    }

    // Copy A = B
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhA ( *A[i] );
        adhereTo<double> adhB ( *B[i] );

        double *a = adhA;
        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            a[j] = b[j];
        }
    }

    test.addTimeMeasurement();

#ifdef PTEST_CHECKS
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        adhereTo<double> adhB ( *B[i] );

        double *b = adhB;

        for ( global_bytesize j = 0; j < size; ++j ) {
            if ( a[j] != j || b[j] != j ) {
                printf ( "Failed check!\n" );
            }
        }
    }
#endif

    // Delete
    #pragma omp parallel for
    for ( global_bytesize i = 0; i < size; ++i ) {
        delete A[i];
        delete B[i];
    }

    test.addTimeMeasurement();
}

string matrixDoubleCopyOpenMPTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation \\\\& Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Copy\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}
#endif


TESTSTATICS ( measureThroughputTest, "Measures throughput under load" );

measureThroughputTest::measureThroughputTest() : performanceTest<int, int> ( "MeasureThroughput" )
{
    TESTPARAM ( 1, 1024, 1024000, 20, true, 1024000, "Byte size per used chunk" );
    TESTPARAM ( 2, 1, 200, 20, true, 100, "percentage of array that will be written to" );
    plotParts = vector<string> ( {"Set Use", "Prepare", "Calculation"} );
}

/// @todo PTEST_CHECKS is missing in this test
void measureThroughputTest::actualTestMethod ( tester &test, int bytesize , int load )
{
    rambrainglobals::config.resizeMemory ( bytesize * 2 );
    rambrainglobals::config.resizeSwap ( bytesize * 2 );

    managedPtr<char> ptr[3] = {managedPtr<char>  ( bytesize ), managedPtr<char>  ( bytesize ), managedPtr<char>  ( bytesize ) };
    adhereTo<char> *adh[3];

    float rewritetimes = load < 0 ? 1 : ( float ) load / 100.;
    int iterations = 10000;

    adh[0] = new adhereTo<char> ( ptr[0] ); //Request element to prepare

    std::chrono::duration<double> allSetuse ( 0 );
    std::chrono::duration<double> allPrepare ( 0 );
    std::chrono::duration<double> allCalc ( 0 );

    using namespace std::chrono;
    for ( int i = 0; i < iterations; ++i ) {
        unsigned int use = ( i % 3 );
        unsigned int prepare = ( ( i + 1 ) % 3 );


        //Important: First say what you will use, then say, what you will use next!

        high_resolution_clock::time_point t0 = high_resolution_clock::now();
        char *loc = * ( adh[use] ); //Actually use the stuff.
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        if ( i != iterations - 1 ) {
            adh[prepare] = new adhereTo<char> ( ptr[prepare] , true );    //Request element to prepare
        }
        high_resolution_clock::time_point t2 = high_resolution_clock::now();

        for ( int r = 0; r < rewritetimes * bytesize; r++ ) {
            loc[r % bytesize] = r * i;
        }
        high_resolution_clock::time_point t3 = high_resolution_clock::now();

        std::chrono::duration<double> setuse = duration_cast<duration<double>> ( t1 - t0 );;
        std::chrono::duration<double> preparet = duration_cast<duration<double>> ( t2 - t1 );
        std::chrono::duration<double> calc = duration_cast<duration<double>> ( t3 - t2 );
        if ( load < 0 ) {
            rewritetimes *= ( preparet + setuse ) > calc ? 1.01 : .99;
        }

        allSetuse += setuse;
        allPrepare += preparet;
        allCalc += calc;
        delete adh[use];
    }
    test.addExternalTime ( allSetuse );
    test.addExternalTime ( allPrepare );
    test.addExternalTime ( allCalc );
}
string measureThroughputTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"SetUse\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Prepare\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Calculation\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($5*100/($3+$4+$5)) with lines title \"busy time in \%\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}
TESTSTATICS ( measurePreemptiveSpeedupTest, "Measures preemptive vs non preemptive runtime" );

measurePreemptiveSpeedupTest::measurePreemptiveSpeedupTest() : performanceTest<int, int> ( "MeasurePreemptiveSpeedup" )
{
    TESTPARAM ( 1, 1024, 1024000, 20, true, 1024000, "Byte size per used chunk" );
    TESTPARAM ( 2, 1, 200, 20, true, 100, "percentage of array that will be written to" );
    plotParts = vector<string> ( {"Set Use", "Prepare", "Calculation", \
                                  "Set Use *", "Prepare *", "Calculation *"
                                 } );
}

/// @todo PTEST_CHECKS is missing in this test
void measurePreemptiveSpeedupTest::actualTestMethod ( tester &test, int bytesize , int load )
{
    unsigned int numel = 1024;
    rambrainglobals::config.resizeMemory ( numel / 2 * bytesize );
    rambrainglobals::config.resizeSwap ( numel * bytesize );

    managedPtr<managedPtr<char>> arr ( numel, bytesize );
    ADHERETOLOC ( managedPtr<char>, arr, ptr );

    float rewritetimes = load < 0 ? 1 : ( float ) load / 100.;
    int iterations = 10230;

    std::chrono::duration<double> allSetuse ( 0 );
    std::chrono::duration<double> allPrepare ( 0 );
    std::chrono::duration<double> allCalc ( 0 );

    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveLoading ( true );
    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveUnloading ( true );

    using namespace std::chrono;
    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveLoading ( true );
    for ( int i = 0; i < iterations; ++i ) {
        unsigned int use = ( i % numel );

        //Important: First say what you will use, then say, what you will use next!
        high_resolution_clock::time_point t0 = high_resolution_clock::now();
        adhereTo<char> glue ( ptr[use] );
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        char *loc = glue; //Actually use the stuff.
        high_resolution_clock::time_point t2 = high_resolution_clock::now();

        for ( int r = 0; r < rewritetimes * bytesize; r++ ) {
            loc[r % bytesize] = r * i;
        }
        high_resolution_clock::time_point t3 = high_resolution_clock::now();

        std::chrono::duration<double> setuse = duration_cast<duration<double>> ( t1 - t0 );
        std::chrono::duration<double> preparet = duration_cast<duration<double>> ( t2 - t1 );
        std::chrono::duration<double> calc = duration_cast<duration<double>> ( t3 - t2 );
        if ( load < 0 ) {
            rewritetimes *= ( preparet + setuse ) > calc ? 1.01 : .99;
        }

        allSetuse += setuse;
        allPrepare += preparet;
        allCalc += calc;
    }

    test.addExternalTime ( allSetuse );
    test.addExternalTime ( allPrepare );
    test.addExternalTime ( allCalc );

    std::chrono::duration<double> allSetuse2 ( 0 );
    std::chrono::duration<double> allPrepare2 ( 0 );
    std::chrono::duration<double> allCalc2 ( 0 );

    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveLoading ( false );
    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveUnloading ( false );
    for ( int i = 0; i < iterations; ++i ) {
        unsigned int use = ( i % numel );

        //Important: First say what you will use, then say, what you will use next!
        high_resolution_clock::time_point t0 = high_resolution_clock::now();
        adhereTo<char> glue ( ptr[use] );
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        char *loc = glue; //Actually use the stuff.
        high_resolution_clock::time_point t2 = high_resolution_clock::now();

        for ( int r = 0; r < rewritetimes * bytesize; r++ ) {
            loc[r % bytesize] = r * i;
        }
        high_resolution_clock::time_point t3 = high_resolution_clock::now();

        std::chrono::duration<double> setuse = duration_cast<duration<double>> ( t1 - t0 );
        std::chrono::duration<double> preparet = duration_cast<duration<double>> ( t2 - t1 );
        std::chrono::duration<double> calc = duration_cast<duration<double>> ( t3 - t2 );
        if ( load < 0 ) {
            rewritetimes *= ( preparet + setuse ) > calc ? 1.01 : .99;
        }

        allSetuse2 += setuse;
        allPrepare2 += preparet;
        allCalc2 += calc;
    }

    test.addExternalTime ( allSetuse2 );
    test.addExternalTime ( allPrepare2 );
    test.addExternalTime ( allCalc2 );

}

string measurePreemptiveSpeedupTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"adhereTo<>\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"type *ptr = glue\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Calculation\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":(100-($5*100/($3+$4+$5))) with lines title \"idle time in \%\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":6 with lines lt 0 lc 1 title \"adhereTo<> *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":7 with lines lt 0 lc 2 title \"type *ptr = glue *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":8 with lines lt 0 lc 3 title \"Calculation *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":(100-($8*100/($6+$7+$8))) with lines lt 0 lc 4 title \"idle time * in \%\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($6+$7+$8) with lines lt 0 lc 5 title \"Total *\"";
    return ss.str();
}

TESTSTATICS ( measureExplicitAsyncSpeedupTest, "Measures runtime of preemptive versus non preemptive with explicite asynchronous preparation" );

measureExplicitAsyncSpeedupTest::measureExplicitAsyncSpeedupTest() : performanceTest<int, int> ( "MeasureExplicitAsyncSpeedup" )
{
    TESTPARAM ( 1, 1024, 1024000, 20, true, 102400, "Byte size per used chunk" );
    TESTPARAM ( 2, 1, 200, 20, true, 100, "percentage of array that will be written to" );
    plotParts = vector<string> ( {"Set Use", "Prepare", "Calculation", "Deletion", \
                                  "Set Use *", "Prepare *", "Calculation *", "Deletion *"
                                 } );
}

/// @todo PTEST_CHECKS is missing in this test
void measureExplicitAsyncSpeedupTest::actualTestMethod ( tester &test, int bytesize , int load )
{
    unsigned int numel = 1024;
    rambrainglobals::config.resizeMemory ( numel / 2 * bytesize );
    rambrainglobals::config.resizeSwap ( numel * bytesize );

    managedPtr<managedPtr<char>> arr ( numel, bytesize );
    ADHERETOLOC ( managedPtr<char>, arr, ptr );

    float rewritetimes = load < 0 ? 1 : ( float ) load / 100.;
    int iterations = 10230;



    std::chrono::duration<double> allSetuse ( 0 );
    std::chrono::duration<double> allPrepare ( 0 );
    std::chrono::duration<double> allCalc ( 0 );



    using namespace std::chrono;
    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveLoading ( true );
    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveUnloading ( true );
    for ( int i = 0; i < iterations; ++i ) {
        unsigned int use = ( i % numel );


        //Important: First say what you will use, then say, what you will use next!
        high_resolution_clock::time_point t0 = high_resolution_clock::now();
        adhereTo<char> glue ( ptr[use] );
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        char *loc = glue; //Actually use the stuff.
        high_resolution_clock::time_point t2 = high_resolution_clock::now();

        for ( int r = 0; r < rewritetimes * bytesize; r++ ) {
            loc[r % bytesize] = r * i;
        }
        high_resolution_clock::time_point t3 = high_resolution_clock::now();

        std::chrono::duration<double> setuse = duration_cast<duration<double>> ( t1 - t0 );;
        std::chrono::duration<double> preparet = duration_cast<duration<double>> ( t2 - t1 );
        std::chrono::duration<double> calc = duration_cast<duration<double>> ( t3 - t2 );
        if ( load < 0 ) {
            rewritetimes *= ( preparet + setuse ) > calc ? 1.01 : .99;
        }

        allSetuse += setuse;
        allPrepare += preparet;
        allCalc += calc;
    }

    test.addExternalTime ( allSetuse );
    test.addExternalTime ( allPrepare );
    test.addExternalTime ( allCalc );

    std::chrono::duration<double> allSetuse2 ( 0 );
    std::chrono::duration<double> allPrepare2 ( 0 );
    std::chrono::duration<double> allCalc2 ( 0 );
    std::chrono::duration<double> allDel ( 0 );

    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveLoading ( false );
    ( ( cyclicManagedMemory * ) managedMemory::defaultManager )->setPreemptiveUnloading ( false );
    adhereTo <char> *adharr[numel];
    adharr[0] = new adhereTo<char> ( ptr[0] );

    for ( int i = 0; i < iterations; ++i ) {
        unsigned int use = ( i % numel );
        unsigned int prepare = ( ( i + 1 ) % numel );


        //Important: First say what you will use, then say, what you will use next!
        high_resolution_clock::time_point t0 = high_resolution_clock::now();
        adharr[prepare] = new adhereTo<char> ( ptr[prepare] );
        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        char *loc = *adharr[use]; //Actually use the stuff.
        high_resolution_clock::time_point t2 = high_resolution_clock::now();

        for ( int r = 0; r < rewritetimes * bytesize; r++ ) {
            loc[r % bytesize] = r * i;
        }
        high_resolution_clock::time_point t3 = high_resolution_clock::now();
        delete adharr[use];
        high_resolution_clock::time_point t4 = high_resolution_clock::now();
        std::chrono::duration<double> setuse = duration_cast<duration<double>> ( t1 - t0 );;
        std::chrono::duration<double> preparet = duration_cast<duration<double>> ( t2 - t1 );
        std::chrono::duration<double> calc = duration_cast<duration<double>> ( t3 - t2 );
        std::chrono::duration<double> del = duration_cast<duration<double>> ( t4 - t3 );
        if ( load < 0 ) {
            rewritetimes *= ( preparet + setuse ) > calc ? 1.01 : .99;
        }

        allSetuse2 += setuse;
        allPrepare2 += preparet;
        allCalc2 += calc;
        allDel += del;
    }

    test.addExternalTime ( allSetuse2 );
    test.addExternalTime ( allPrepare2 );
    test.addExternalTime ( allCalc2 );
    test.addExternalTime ( allDel );

}

string measureExplicitAsyncSpeedupTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"adhereTo<>\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"type *ptr = glue\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Calculation\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":(100-($5*100/($3+$4+$5))) with lines title \"idle time in \%\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":6 with lines lt 0 lc 1 title \"adhereTo<> *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":7 with lines lt 0 lc 2 title \"type *ptr = glue *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":8 with lines lt 0 lc 3 title \"Calculation *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":9 with lines lt 0 lc 3 title \"adhereTo<> deletion *\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":(100-($8*100/($6+$7+$8+$9))) with lines lt 0 lc 4 title \"idle time * in \%\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($6+$7+$8+$9) with lines lt 0 lc 5 title \"Total *\"";
    return ss.str();
}

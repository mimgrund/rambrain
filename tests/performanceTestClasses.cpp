#include "performanceTestClasses.h"

map<string, performanceTest<> *> performanceTest<>::testClasses;

performanceTest<>::performanceTest ( const char *name ) : name ( name )
{
    testClasses[name] = this;
}

void performanceTest<>::runTests ( unsigned int repetitions, const string &path  )
{
    cout << "Running test case " << name << std::endl;
    for ( int param = parameters.size() - 1; param >= 0; --param ) {
        if ( parameters[param]->enabled ) {
            unsigned int steps = getStepsForParam ( param );
            ofstream temp ( "temp.dat" );

            for ( unsigned int step = 0; step < steps; ++step ) {
                string params = getParamsString ( param, step );
                stringstream call;
                call << path << "membrain-performancetests " << repetitions << " " << name << " " << params << " 2> /dev/null";
                cout << "Calling: " << call.str() << endl;
                system ( call.str().c_str() );

                resultToTempFile ( param, step, temp );
                temp << endl;

                handleTimingInfos ( param, step );
            }

            temp.close();
            ofstream gnutemp ( "temp.gnuplot" );
            stringstream outname;
            outname << name << param;
            cout << "Generating output file " << outname.str() << endl;
            gnutemp << generateGnuplotScript ( outname.str(), parameters[param]->name, "Execution time [ms]", name, parameters[param]->deltaLog, parameters.size() - param );
            gnutemp.close();

            cout << "Calling gnuplot and displaying result" << endl;
            system ( "gnuplot temp.gnuplot" );
            system ( ( "convert -density 300 -resize 1920x " + outname.str() + ".eps -flatten " + outname.str() + ".png" ).c_str() );
            system ( ( "display " + outname.str() + ".png &" ).c_str() );
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
    ss << "perftest_" << name;
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
            stringstream ss ( line );
            vector<string> parts;
            string part;
            while ( getline ( ss, part, '\t' ) ) {
                parts.push_back ( part );
            }
            file << parts[parts.size() - 2] << '\t';
        }
    }
}

string performanceTest<>::generateGnuplotScript ( const string &name, const string &xlabel, const string &ylabel, const string &title, bool log, int paramColumn )
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
    ss << generateMyGnuplotPlotPart ( "temp.dat", paramColumn );
    return ss.str();
}

void performanceTest<>::handleTimingInfos ( int varryParam, unsigned int step )
{
    // Move stats file for permanent storage
    string outFile = getTestOutfile ( varryParam, step );
    string timingFile = outFile + "_stats";
    string tempFile = "timingTemp.dat";
    system ( ( "mv membrain-swapstats.log " + timingFile ).c_str() );

    //! \todo can the last line be corrupted? evtl check for this and remove it

    //! \todo handle multiple repetitions
    ifstream test ( outFile );
    ifstream timing ( timingFile );
    ofstream out ( tempFile );

    // Go through test output and get first pair of times
    //! \todo this can be largely refactored
    string testLine, timingLine;
    while ( getline ( test, testLine ) ) {
        if ( testLine.find ( '#' ) == string::npos ) {
            stringstream ss ( testLine );
            vector<string> testParts;
            string testPart;
            while ( getline ( ss, testPart, '\t' ) ) {
                testParts.push_back ( testPart );
            }

            char *buf;
            unsigned long long start = strtoull ( testParts[1].c_str(), &buf, 10 );
            unsigned long long end = strtoull ( testParts[2].c_str(), &buf, 10 );

            // No go through timing file and look for the matching lines there
            vector<vector<string>> relevantTimingParts;
            int last = 0;
            while ( getline ( timing, timingLine ) ) {
                if ( timingLine.find ( '#' ) == string::npos ) {
                    stringstream ss ( timingLine );
                    vector<string> timingParts;
                    string timingPart;
                    while ( getline ( ss, timingPart, '\t' ) ) {
                        timingParts.push_back ( timingPart );
                    }

                    unsigned long long current = strtoull ( timingParts[0].c_str(), &buf, 10 );
                    if ( current >= start && current <= end ) {
                        relevantTimingParts.push_back ( timingParts );
                    }
                    if ( current > end ) {
                        timing.seekg ( last );
                        break;
                    }

                    last = timing.tellg();
                }
            }

            // We have all stats for the current run segment, output this data to the temp file
            for ( auto it = relevantTimingParts.begin(); it != relevantTimingParts.end(); ++it ) {
                unsigned long long relTime = strtoull ( ( *it ) [0].c_str(), &buf, 10 ) - strtoull ( relevantTimingParts.front() [0].c_str(), &buf, 10 );
                unsigned long long mbOut = strtoul ( ( *it ) [1].c_str(), &buf, 10 ) / mib;
                unsigned long long mbIn = strtoul ( ( *it ) [3].c_str(), &buf, 10 ) / mib;

                out << relTime << " " << mbOut << " " << mbIn << " " << ( *it ) [5] << endl;
            }

            //! \todo as long as we dont accumulate we just look at the first time measurement, remove this break then
            break;
        }
    }

    //! \todo This data has acutally to be accumulated, instead at the moment we just plot it as single line

    test.close();
    timing.close();
    out.close();

    //! \todo refactor that, see above
    // Plot that thing

    //! \todo again - this is code duplication -> refactor
    ofstream gnutemp ( "temp.gnuplot" );
    stringstream outname;
    outname << name << varryParam << "_stats";
    cout << "Generating output file " << outname.str() << endl;
    gnutemp << "set terminal postscript eps enhanced color 'Helvetica,10'" << endl;
    gnutemp << "set output \"" << outname.str() << ".eps\"" << endl;
    gnutemp << "set xlabel \"Time [ms]\"" << endl;
    gnutemp << "set ylabel \"Swap Movement [MB]\"" << endl;
    gnutemp << "set title \"" << name << "\"" << endl;
    gnutemp << "set style data linespoints" << endl;
    //! \todo actually the legend comes from the definition of the test class like in the normal plot, make this a general gather
    gnutemp << "plot '" << tempFile << "' using 1:2 lt -1 pt 4 lc 1 title \"Swapped out\", \\" << endl;
    gnutemp << "'" << tempFile << "' using 1:3 lt -1 pt 4 lc 2 title \"Swapped in\"" << endl;
    //! \todo what about hit/miss plot?
    gnutemp.close();

    cout << "Calling gnuplot and displaying result" << endl;
    system ( "gnuplot temp.gnuplot" );
    system ( ( "convert -density 300 -resize 1920x " + outname.str() + ".eps -flatten " + outname.str() + ".png" ).c_str() );
    system ( ( "display " + outname.str() + ".png &" ).c_str() );

    //! \todo just for debug
    exit ( 2 );
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
}

void matrixTransposeTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 2;

    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );

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


    // Delete
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }

    test.addTimeMeasurement();
}

string matrixTransposeTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation & Definition\", \\" << endl;
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
}

void matrixCleverTransposeTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 2;

    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );

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

//     for ( unsigned int i = 0; i < size; ++i ) {
//         adhereTo<double> rowloc ( *rows[i] );
//         double *rowdbl =  rowloc;
//         for ( unsigned int j = 0; j < size; ++j ) {
//              if(rowdbl[j] != j * size + i)
//                  printf("Failed check!");
//         }
//     }

    // Delete
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }

    test.addTimeMeasurement();
}

string matrixCleverTransposeTest::generateMyGnuplotPlotPart ( const string &file , int paramColumn )
{
    stringstream ss;
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation & Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


TESTSTATICS ( matrixCleverTransposeOpenMPTest, "Same as cleverTranspose, but with OpenMP" );

matrixCleverTransposeOpenMPTest::matrixCleverTransposeOpenMPTest() : performanceTest<int, int> ( "MatrixCleverTransposeOpenMP" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
}

void matrixCleverTransposeOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    //managedFileSwap swap ( swapmem, "membrainswap-%d" );
    //cyclicManagedMemory manager ( &swap, mem );
    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );

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

    //     for ( unsigned int i = 0; i < size; ++i ) {
    //         adhereTo<double> rowloc ( *rows[i] );
    //         double *rowdbl =  rowloc;
    //         for ( unsigned int j = 0; j < size; ++j ) {
    //              if(rowdbl[j] != j * size + i)
    //                  printf("Failed check!");
    //         }
    //     }

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
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation & Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


TESTSTATICS ( matrixCleverBlockTransposeOpenMPTest, "Same as cleverTranspose, but with OpenMP and blockwise multiplication" );

matrixCleverBlockTransposeOpenMPTest::matrixCleverBlockTransposeOpenMPTest() : performanceTest<int, int> ( "MatrixCleverBlockTransposeOpenMP" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 1000, 10000, 20, true, 2000, "Matrix rows in main memory" );
}

void matrixCleverBlockTransposeOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    //managedFileSwap swap ( swapmem, "membrainswap-%d" );
    //cyclicManagedMemory manager ( &swap, mem );
    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );


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
//     for ( unsigned int i = 0; i < size; ++i ) {
//         for ( unsigned int j = 0; j < size; ++j ) {
//             unsigned int blckidx = blockIdx(i,j);
//             unsigned int inblck = inBlockIdx(i,j);
//             adhereTo<double> adh(*rows[blckidx]);
//             double * loc = adh;
//             //             if(loc[inblck] != j * size + i)
//             //                 printf("Failed check!");
//             //printf("%d,%d\t",blckidx,inblck);
//             printf("%e\t",loc[inblck]);
//         }
//         printf("\n");
//     }

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
// //      printf("\n");
//     for ( unsigned int i = 0; i < size; ++i ) {
//         for ( unsigned int j = 0; j < size; ++j ) {
//             unsigned int blckidx = blockIdx(i,j);
//             unsigned int inblck = inBlockIdx(i,j);
//             adhereTo<double> adh(*rows[blckidx]);
//             double * loc = adh;
//                         if(loc[inblck] != j * size + i)
//                             printf("Failed check!");
// //             printf("%d,%d\t",blckidx,inblck);
// //             printf("%e\t",loc[inblck]);
//         }
// //         printf("\n");
//     }




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
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation & Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Transposition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


TESTSTATICS ( matrixMultiplyTest, "Matrix multiplication with matrices being stored in columns / rows" );

matrixMultiplyTest::matrixMultiplyTest() : performanceTest<int, int> ( "MatrixMultiply" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 4000, 30000, 20, true, 12000, "Matrix rows in main memory" );
}

void matrixMultiplyTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );


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
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation & Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Multiplication\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}


TESTSTATICS ( matrixMultiplyOpenMPTest, "Matrix multiplication with matrices being stored in columns / rows" );

matrixMultiplyOpenMPTest::matrixMultiplyOpenMPTest() : performanceTest<int, int> ( "MatrixMultiplyOpenMP" )
{
    TESTPARAM ( 1, 10, 8000, 20, true, 4000, "Matrix size per dimension" );
    TESTPARAM ( 2, 4000, 30000, 20, true, 12000, "Matrix rows in main memory" );
}

void matrixMultiplyOpenMPTest::actualTestMethod ( tester &test, int param1, int param2 )
{
    const global_bytesize size = param1;
    const global_bytesize memlines = param2;
    const global_bytesize mem = size * sizeof ( double ) *  memlines;
    const global_bytesize swapmem = size * size * sizeof ( double ) * 4;

    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );


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
    ss << "plot '" << file << "' using " << paramColumn << ":3 with lines title \"Allocation & Definition\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":4 with lines title \"Multiplication\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":5 with lines title \"Deletion\", \\" << endl;
    ss << "'" << file << "' using " << paramColumn << ":($3+$4+$5) with lines title \"Total\"";
    return ss.str();
}

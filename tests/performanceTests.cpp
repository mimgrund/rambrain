#include <iostream>
#include <vector>
#include <cstring>

#include "tester.h"

#include "managedFileSwap.h"
#include "cyclicManagedMemory.h"
#include "managedPtr.h"
#include "membrainconfig.h"

using namespace std;
using namespace membrain;

void runMatrixTranspose ( tester *test, char **args );
void runMatrixCleverTranspose ( tester *test, char **args );
void runMatrixCleverTransposeOpenMP ( tester *test, char **args );

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
    tests.push_back ( testMethod ( "MatrixCleverTranspose", 2, runMatrixCleverTranspose, "Measurements of allocation and definition, transposition, deletion times, but with a clever transposition algorithm" ) );
    tests.push_back ( testMethod ( "MatrixCleverTransposeOpenMP", 2, runMatrixCleverTransposeOpenMP, "Same as cleverTranspose, but with OpenMP" ) );

    int i = 1;
    if ( i >= argc ) {
        std::cerr << "Not enough arguments supplied, expected number of repetitions followed by test directives, exiting" << std::endl;
        return 2;
    }

    int repetitions = atoi ( argv[i++] );

    while ( i < argc ) {
        std::cout << "Attempting to run " << argv[i] << std::endl;

        for ( vector<testMethod>::iterator it = tests.begin(); it != tests.end(); ++it ) {
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
                    myTester.startNewCycle();
                    it->test ( &myTester, args );
                }

                myTester.writeToFile();

                if ( it->argumentCount > 0 ) {
                    delete[] args;
                }
                break;
            }
        }
    }

    std::cout << "Performance tests done" << std::endl;
    return 0;
}


void runMatrixTranspose ( tester *test, char **args )
{
    const unsigned int size = atoi ( args[0] );
    const unsigned int memlines = atoi ( args[1] );
    const unsigned int mem = size * sizeof ( double ) *  memlines;
    const unsigned int swapmem = size * size * sizeof ( double ) * 2;

    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );

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

void runMatrixCleverTranspose ( tester *test, char **args )
{
    const unsigned int size = atoi ( args[0] );
    const unsigned int memlines = atoi ( args[1] );
    const unsigned int mem = size * sizeof ( double ) *  memlines;
    const unsigned int swapmem = size * size * sizeof ( double ) * 2;

    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );

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

    test->addTimeMeasurement();

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

    test->addTimeMeasurement();
}

void runMatrixCleverTransposeOpenMP ( tester *test, char **args )
{
    const unsigned int size = atoi ( args[0] );
    const unsigned int memlines = atoi ( args[1] );
    const unsigned int mem = size * sizeof ( double ) *  memlines;
    const unsigned int swapmem = size * size * sizeof ( double ) * 4;

    //managedFileSwap swap ( swapmem, "membrainswap-%d" );
    //cyclicManagedMemory manager ( &swap, mem );
    membrainglobals::config.resizeMemory ( mem );
    membrainglobals::config.resizeSwap ( swapmem );

    test->addTimeMeasurement();

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

    test->addTimeMeasurement();

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


    test->addTimeMeasurement();

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

    test->addTimeMeasurement();
}

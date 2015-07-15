#include <iostream>
#include <mpi.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"

using namespace std;
using namespace membrain;

int main ( int argc, char **argv )
{
    const unsigned int dblamount = 100;
    const global_bytesize mem = dblamount * 2.5 * sizeof ( double );
    const global_bytesize swapmem = 100 * dblamount * sizeof ( double );
    int ret = 0;

    MPI_Init ( &argc, &argv );

    int tasks = 5;
    MPI_Comm_size ( MPI_COMM_WORLD, &tasks );
    cout << "Starting mpi test on " << tasks << " tasks" << endl;

    managedFileSwap swap ( swapmem, "/tmp/membrainswap-%d-%d" );
    cyclicManagedMemory manager ( &swap, mem );

    managedPtr<double> data1 ( dblamount );
    managedPtr<double> data3 ( dblamount );
    {
        ADHERETOLOC ( double, data1, d1 );
        for ( unsigned int i = 0; i < dblamount; ++i ) {
            d1[i] = i * 1.0;
        }
    }

    managedPtr<double> data2 ( dblamount );
    {
        ADHERETOLOC ( double, data2, d2 );
        for ( unsigned int i = 0; i < dblamount; ++i ) {
            d2[i] = i * dblamount * 1.0;
        }
    }

    {
        ADHERETOLOC ( double, data1, d1 );
        ADHERETOLOC ( double, data3, d3 );
        MPI_Allreduce ( d1, d3, dblamount, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
        for ( unsigned int i = 0; i < dblamount; ++i ) {
            d1[i] = d3[i];
        }
    }

    {
        ADHERETOLOC ( double, data2, d2 );
        ADHERETOLOC ( double, data3, d3 );
        MPI_Allreduce ( d2, d3, dblamount, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
        for ( unsigned int i = 0; i < dblamount; ++i ) {
            d2[i] = d3[i];
        }
    }

    {
        ADHERETOLOC ( double, data1, d1 );
        for ( unsigned int i = 0; i < dblamount; ++i ) {
            if ( d1[i] != i * 1.0 * tasks ) {
                cerr << "Result of allreduce not correct! Expected " << i * 1.0 * tasks << " at index d1[" << i << "], got " << d1[i] << endl;
                ++ ret;
            }
        }
    }

    {
        ADHERETOLOC ( double, data2, d2 );
        for ( unsigned int i = 0; i < dblamount; ++i ) {
            if ( d2[i] != i * 1.0 * tasks * dblamount ) {
                cerr << "Result of allreduce not correct! Expected " << i * 1.0 * dblamount *tasks << " at index d2[" << i << "], got " << d2[i] << endl;
                ++ ret;
            }
        }
    }

    MPI_Finalize();

    cout << "Test done" << endl;

    return ret;
}

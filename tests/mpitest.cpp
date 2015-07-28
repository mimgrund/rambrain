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

#include <iostream>
#include <mpi.h>
#include "managedPtr.h"
#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"

using namespace std;
using namespace rambrain;

/**
 * @brief Provides a test binary to check the behaviour of mpi in combination with the lib
 */
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

    managedFileSwap swap ( swapmem, "/tmp/rambrainswap-%d-%d" );
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


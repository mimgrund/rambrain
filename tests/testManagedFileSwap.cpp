#include "tester.h"
IGNORE_TEST_WARNINGS;

#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"
#include "managedPtr.h"
#include "managedDummySwap.h"
#include "dummyManagedMemory.h"
#include <gtest/gtest.h>
#include <sys/stat.h>
#include "common.h"
#include <chrono>

using namespace rambrain;

/// @todo Write a test for const lazy allocation / deallocation

/**
 * @test Tests whether managedFileSwap can take a memoryChunk and store it securely.
 */
TEST ( managedFileSwap, Unit_ManualSwapping ) //This screws default manager, as byte accounting is done asynchronously (invoked by managedFileSwap...)
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedFileSwap swap ( swapmem, "/tmp/rambrainswap-%d-%d" );
    //Protect default manager from manipulations:
    managedDummySwap dummyswap ( gig );
    cyclicManagedMemory dummymanager ( &dummyswap, gig );

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = new managedMemoryChunk ( 0, 1 );
#else
    managedMemoryChunk *chunk = new managedMemoryChunk ( 1 );
#endif
    chunk->status = MEM_ALLOCATED;
    chunk->locPtr = malloc ( dblsize );
    chunk->size = dblsize;

    pthread_mutex_lock ( & ( managedMemory::stateChangeMutex ) );
    EXPECT_TRUE ( swap.swapOut ( chunk ) );

    swap.waitForCleanExit();
    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    EXPECT_TRUE ( swap.swapIn ( chunk ) ) ;
    swap.waitForCleanExit();
    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    pthread_mutex_unlock ( & ( managedMemory::stateChangeMutex ) );
    free ( chunk->locPtr );
    delete chunk;
}

/**
 * @test Tests whether managedFileSwap can take a few memoryChunk and store it securely.
 */
TEST ( managedFileSwap, Unit_ManualMultiSwapping )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedFileSwap swap ( swapmem, "/tmp/rambrainswap-%d-%d" );

    //Protect default manager from manipulations:
    managedDummySwap dummyswap ( gig );
    cyclicManagedMemory dummymanager ( &dummyswap, gig );

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    pthread_mutex_lock ( & ( managedMemory::stateChangeMutex ) );
    managedMemoryChunk *chunks[2];
    for ( int i = 0; i < 2; ++i ) {
#ifdef PARENTAL_CONTROL
        chunks[i] = new managedMemoryChunk ( 0, i + 1 );
#else
        chunks[i] = new managedMemoryChunk ( i + 1 );
#endif

        chunks[i]->status = MEM_ALLOCATED;
        chunks[i]->locPtr = malloc ( dblsize );
        chunks[i]->size = dblsize;
    }

    swap.swapOut ( chunks, 2 );
    swap.waitForCleanExit();
    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 2 * dblsize, swap.getUsedSwap() );
    swap.swapIn ( chunks, 2 );
    swap.waitForCleanExit();
    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    for ( int i = 0; i < 2; ++i ) {
        if ( chunks[i]->status == MEM_SWAPPED ) {
            free ( chunks[i]->swapBuf );
        } else {
            free ( chunks[i]->locPtr );
        }
        delete chunks[i];
    }
    pthread_mutex_unlock ( & ( managedMemory::stateChangeMutex ) );
}

/**
 * @test Tests whether deletion of swapped out elements is handled correctly.
 */
TEST ( managedFileSwap, Unit_ManualSwappingDelete )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    managedFileSwap swap ( swapmem, "/tmp/rambrainswap-%d-%d" );

    //Protect default manager from manipulations:
    managedDummySwap dummyswap ( gig );
    cyclicManagedMemory dummymanager ( &dummyswap, gig );

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );
    pthread_mutex_lock ( & ( managedMemory::stateChangeMutex ) );
#ifdef PARENTAL_CONTROL
    managedMemoryChunk *chunk = new managedMemoryChunk ( 0, 1 );
#else
    managedMemoryChunk *chunk = new managedMemoryChunk ( 1 );
#endif
    chunk->status = MEM_ALLOCATED;
    chunk->locPtr =  malloc ( dblsize );
    chunk->size = dblsize;

    swap.swapOut ( chunk );
    swap.waitForCleanExit();

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    swap.swapDelete ( chunk );
    swap.waitForCleanExit();

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    delete chunk;
    pthread_mutex_unlock ( & ( managedMemory::stateChangeMutex ) );
}
/**
 * @test Tests interplay between managedFileSwap and a cylicManaged Memory
 */
TEST ( managedFileSwap, Unit_SimpleSwapping )
{
    const unsigned int memsize = 15 * sizeof ( double );
    const unsigned int swapsize = 10 * memsize;

    ASSERT_NO_THROW (
        managedFileSwap swap ( swapsize, "rambrainswap-%d-%d" );
        cyclicManagedMemory manager ( &swap, memsize );

        managedPtr<double> ptr1 ( 10 );
        managedPtr<double> ptr2 ( 10 );
    );
}

/**
 * @test Tests whether the swap size is accounted for correctly.
 */
TEST ( managedFileSwap, Unit_SwapSize )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    const unsigned int memsize = dblsize * 1.5;
    managedFileSwap swap ( swapmem, "/tmp/rambrainswap-%d-%d" );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    managedPtr<double> *ptr1 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    ptr1->setUse();

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    ptr1->unsetUse();

    managedPtr<double> *ptr2 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    ptr2->setUse();

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    ptr2->unsetUse();

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    delete ptr1;

    ASSERT_EQ ( mib, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    delete ptr2;
}

/**
 * @test Puts memory manager and swap under heavy load of objects of the same size by randomly allocating / deallocating them
 */
TEST ( managedFileSwap, Integration_RandomAccess )
{
    global_bytesize oneswap = 1024 * 1024 * ( global_bytesize ) 16;
    global_bytesize totalswap = 16 * oneswap;
    tester test;
    test.setSeed();

    managedFileSwap swap ( totalswap, "./rambrainswap-%d-%d", oneswap );
    cyclicManagedMemory manager ( &swap, oneswap );

    /*ASSERT_EQ ( 0, manager.getSwappedMemory() ); //Deallocated all pointers
    ASSERT_EQ ( 0, swap.getUsedSwap() );
    ASSERT_EQ ( 16, swap.all_space.size() );
    ASSERT_EQ ( 16, swap.free_space.size() );*/
    infomsgf ( "%ld total swap in %ld swapfiles", swap.getSwapSize(), swap.all_space.size() );

    global_bytesize obj_size = 102400 * sizeof ( double );
    global_bytesize obj_no = totalswap / obj_size * .9;


    managedPtr<double> **objmask = ( managedPtr<double> ** ) malloc ( sizeof ( managedPtr<double> * ) *obj_no );
    for ( unsigned int n = 0; n < obj_no; ++n ) {
        objmask[n] = NULL;
    }
    for ( unsigned int n = 0; n < 10 * obj_no; ++n ) {
        global_bytesize no = test.random ( obj_no );

        if ( objmask[no] == NULL ) {
            ASSERT_TRUE ( manager.checkCycle() );
            objmask[no] = new managedPtr<double> ( 102400 );
            {
                adhereTo<double> objoloc ( *objmask[no] );
                double *darr =  objoloc;
                darr[0] = no;
            }

        } else {
            {
                adhereTo<double> objoloc ( *objmask[no] );
                double *darr =  objoloc;
                ASSERT_EQ ( no, darr[0] );
            }
            delete objmask[no];
            objmask[no] = NULL;
        }

    }
    for ( unsigned int n = 0; n < obj_no; ++n ) {
        if ( objmask[n] != NULL ) {
            delete objmask[n];
        }
    }
    free ( objmask );

#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

}
/**
 * @test Puts memory manager and swap under heavy load of objects of various sizes by randomly allocating / deallocating them
 */
TEST ( managedFileSwap, Integration_RandomAccessVariousSize )
{
    global_bytesize oneswap = 1024 * 1024 * ( global_bytesize ) 16;
    global_bytesize totalswap = 32 * oneswap;
    tester test;
    test.setSeed();

    managedFileSwap swap ( totalswap, "./rambrainswap-%d-%d", oneswap );
    cyclicManagedMemory manager ( &swap, oneswap );

    /*ASSERT_EQ ( 0, manager.getSwappedMemory() ); //Deallocated all pointers
     *   ASSERT_EQ ( 0, swap.getUsedSwap() );
     *   ASSERT_EQ ( 16, swap.all_space.size() );
     *   ASSERT_EQ ( 16, swap.free_space.size() );*/
    infomsgf ( "%ld total swap in %ld swapfiles", swap.getSwapSize(), swap.all_space.size() );

    global_bytesize obj_size = 102400 * sizeof ( double );
    global_bytesize obj_no = totalswap / obj_size * 2;

    managedPtr<double> **objmask = ( managedPtr<double> ** ) malloc ( sizeof ( managedPtr<double> * ) *obj_no );
    for ( unsigned int n = 0; n < obj_no; ++n ) {
        objmask[n] = NULL;
    }
    for ( unsigned int n = 0; n < 10 * obj_no; ++n ) {

        global_bytesize no = test.random ( obj_no );
        ASSERT_TRUE ( manager.checkCycle() );
        if ( objmask[no] == NULL ) {

            unsigned int varsize = ( test.random() + 0.5 ) * 102400;
            if ( ( varsize + 102400 * 2. ) *sizeof ( double ) > swap.getFreeSwap() ) {
                continue;
            }
            objmask[no] = new managedPtr<double> ( varsize );
            {
                adhereTo<double> objoloc ( *objmask[no] );
                double *darr =  objoloc;
                for ( unsigned int k = 0; k < varsize; k++ ) {
                    darr[k] = k + varsize;
                }
            }

        } else {
            {
                adhereTo<double> objoloc ( *objmask[no] );
                double *darr =  objoloc;
                for ( int k = 0; k < darr[0]; k++ ) {
                    ASSERT_EQ ( k + darr[0], darr[k] );
                }

            }
            delete objmask[no];
            objmask[no] = NULL;
        }
    }
    for ( unsigned int n = 0; n < obj_no; ++n ) {
        if ( objmask[n] != NULL ) {
            delete objmask[n];
        }
    }
    free ( objmask );

#ifdef SWAPSTATS
    manager.printSwapstats();
#endif

}

/**
 * @test Checks correct allocation of swap files
 */
TEST ( managedFileSwap, Unit_SwapAllocation )
{
    unsigned int oneswap = 1024 * 1024 * 16;
    unsigned int totalswap = 16 * oneswap;
    managedFileSwap swap ( totalswap, "/tmp/rambrainswap-%d-%d", oneswap );
    char firstname[70];
    snprintf ( firstname, 70, "/tmp/rambrainswap-%d-%d", getpid(), 0 );

    for ( int n = 0; n < 16; n++ ) {
        char fname[70];
        snprintf ( fname, 70, "/tmp/rambrainswap-%d-%d", getpid(), n );
        struct stat mstat;
        stat ( fname, &mstat );
        ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
        ASSERT_EQ ( 0, mstat.st_size );

    }
    ASSERT_EQ ( 16u, swap.all_space.size() );
    ASSERT_EQ ( 16u, swap.free_space.size() );

    cyclicManagedMemory manager ( &swap, 1024 * 10 );
    {
        managedPtr<double> testarr ( 1024 ); //80% RAM full.
        for ( int n = 0; n < 16; n++ ) {
            char fname[70];
            snprintf ( fname, 70, "/tmp/rambrainswap-%d-%d", getpid(), n );
            struct stat mstat;
            stat ( fname, &mstat );
            ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
            ASSERT_EQ ( 0, mstat.st_size );

        }
        managedPtr<double> testarr2 ( 1024 ); //80% RAM full., swap out occurs.
        struct stat first;
        stat ( firstname, &first );
        ASSERT_TRUE ( S_ISREG ( first.st_mode ) );
        ASSERT_EQ ( ( ( unsigned int ) ( ( 1024 * 1024 * 16 ) * swap.swapFileResizeFrac ) ), first.st_size ); // should be 1/16th pageFileSize
        for ( int n = 1; n < 16; n++ ) {
            char fname[70];
            snprintf ( fname, 70, "/tmp/rambrainswap-%d-%d", getpid(), n );
            struct stat mstat;
            stat ( fname, &mstat );
            ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
            ASSERT_EQ ( 0, mstat.st_size );

        }
        managedPtr<double> testarr3 ( 1024 );
        ASSERT_EQ ( 8 * 1024u * 2, manager.getSwappedMemory() );
        stat ( firstname, &first );
        ASSERT_TRUE ( S_ISREG ( first.st_mode ) );
        ASSERT_EQ ( ( ( unsigned int ) ( ( 1024 * 1024 * 16 ) * swap.swapFileResizeFrac ) ), first.st_size ); //Size should not change.
        for ( int n = 1; n < 16; n++ ) {
            char fname[70];
            snprintf ( fname, 70, "/tmp/rambrainswap-%d-%d", getpid(), n );
            struct stat mstat;
            stat ( fname, &mstat );
            ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
            ASSERT_EQ ( 0, mstat.st_size );

        }

        swap.waitForCleanExit();
    }

    ASSERT_EQ ( 0, manager.getSwappedMemory() ); //Deallocated all pointers
    ASSERT_EQ ( 0, swap.getUsedSwap() );
    ASSERT_EQ ( 16, swap.all_space.size() );
    ASSERT_EQ ( 16, swap.free_space.size() );

    //Start to fill up swap with chunks that are below window size:
    // Wsize = 1MB , 10KiB Objects in RAM, 100 fit exactly in one swapFile.
    //let us have objects that fill RAM completely:
    const unsigned int tenkbdoublearrsize = 1024 / 8 * 10;
    const unsigned int doublearrsize = tenkbdoublearrsize * sizeof ( double );
    managedPtr<double> *ptarr[1641];
    for ( unsigned int n = 0; n < 1639; n++ ) { // Create just as much as fit into RAM+1 swapfile
        ptarr[n] = new managedPtr<double> ( tenkbdoublearrsize );
        ASSERT_TRUE ( manager.checkCycle() );
    };
    ASSERT_EQ ( doublearrsize, manager.getUsedMemory() );
    ASSERT_EQ ( doublearrsize * 1638, manager.getSwappedMemory() );
    for ( unsigned int n = 1; n < 1639; n++ ) { // Create just as much as fit into RAM+1 swapfile
        delete ptarr[n];
    };
    ASSERT_EQ ( 0, manager.getUsedMemory() );
    ASSERT_EQ ( doublearrsize, manager.getSwappedMemory() );
    delete ptarr[0];

    //Fill up over pagefile boundary:
    for ( unsigned int n = 0; n < 1641; n++ ) { // Create just as much as fit into RAM+1 swapfile
        ptarr[n] = new managedPtr<double> ( tenkbdoublearrsize );
    };
    ASSERT_EQ ( doublearrsize, manager.getUsedMemory() );
    ASSERT_EQ ( doublearrsize * 1640, manager.getSwappedMemory() );
    for ( unsigned int n = 0; n < 1641; n++ ) { // Create just as much as fit into RAM+1 swapfile
        delete ptarr[n];
    };
    ASSERT_EQ ( 0, manager.getUsedMemory() );
    ASSERT_EQ ( 0, manager.getSwappedMemory() );
    ASSERT_EQ ( 16, swap.all_space.size() );
    ASSERT_EQ ( 16, swap.free_space.size() );

#ifdef SWAPSTATS
    manager.printSwapstats();
#endif
}


///\todo This test does not work, exception is handled by libc++ instead of gtest This test was written in order to provoke the unfishedCodeException in managedFileSwap.cpp:322 ; What did I do wrong
///\todo Please correct the test such that it fails due to that exception in order to track completeness of the code there...
TEST ( managedFileSwap, Unit_SwapReadAllocatedChunk )
{
    const unsigned int oneswap = mib;
    const unsigned int dblamount = 10;
    const unsigned int arrsize = dblamount * sizeof ( double );
    managedFileSwap swap ( oneswap, "/tmp/rambrainswap-%d-%d", oneswap );
    cyclicManagedMemory manager ( &swap, arrsize * 1.5 );

    managedPtr<double> ptr1 ( dblamount );

    ASSERT_EQ ( arrsize, manager.getUsedMemory() );
    ASSERT_EQ ( 0, manager.getSwappedMemory() );

    managedPtr<double> ptr2 ( dblamount );

    ASSERT_EQ ( arrsize, manager.getUsedMemory() );
    ASSERT_EQ ( arrsize, manager.getSwappedMemory() );

    {
        ADHERETOLOCCONST ( double, ptr1, locptr1 );
    }

    ASSERT_EQ ( arrsize, manager.getUsedMemory() );
    ASSERT_EQ ( arrsize, manager.getSwappedMemory() );

    ASSERT_NO_THROW ( managedPtr<double> ptr3 ( dblamount );
                      ASSERT_EQ ( arrsize, manager.getUsedMemory() );
                      ASSERT_EQ ( 2 * arrsize, manager.getSwappedMemory() ); );
}

/**
 * @test Checks capability to cope with memory shortage in certain circumstances
 */
TEST ( managedFileSwap, Unit_SwapSingleIsland )
{
    const unsigned int oneswap = mib;
    const unsigned int dblamount = oneswap / sizeof ( double );
    const unsigned int smalldblamount = dblamount * .2;
    const unsigned int bigdblamount = dblamount - smalldblamount;
    managedFileSwap swap ( oneswap, "/tmp/rambrainswap-%d-%d", oneswap );
    cyclicManagedMemory manager ( &swap, oneswap );

    ASSERT_EQ ( mib, swap.getSwapSize() );

    //ASSERT_NO_THROW (
    managedPtr<double> ptr1 ( bigdblamount ); // To fill up swap

    ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getUsedMemory() );
    ASSERT_EQ ( 0, manager.getSwappedMemory() );
    managedPtr<double> ptr2 ( bigdblamount ); // To fill up memory

    ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getUsedMemory() );
    ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getSwappedMemory() );

    managedPtr<double> ptr3 ( smalldblamount ); // Can fit in swap

    ASSERT_EQ ( ( smalldblamount + bigdblamount ) * sizeof ( double ), manager.getUsedMemory() );
    ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getSwappedMemory() );

    swap.waitForCleanExit();
    managedPtr<double> ptr4 ( smalldblamount ); // Can fit in remaining memory


    ASSERT_EQ ( ( smalldblamount + bigdblamount ) * sizeof ( double ), manager.getUsedMemory() );
    ASSERT_EQ ( ( smalldblamount + bigdblamount ) * sizeof ( double ), manager.getSwappedMemory() );

    ASSERT_EQ ( MEM_SWAPPED, ptr1.chunk->status );
    ASSERT_EQ ( MEM_ALLOCATED, ptr2.chunk->status );
    ASSERT_EQ ( MEM_SWAPPED, ptr3.chunk->status );
    ASSERT_EQ ( MEM_ALLOCATED, ptr4.chunk->status );
    //);
}


/**
 * @test Checks capability to cope with memory shortage in certain circumstances
 */
TEST ( managedFileSwap, Unit_SwapNextAndSingleIsland )
{
    const unsigned int oneswap = mib;
    const unsigned int dblamount = oneswap / sizeof ( double );
    const unsigned int smalldblamount = dblamount * .1;
    const unsigned int bigdblamount = dblamount - 2 * smalldblamount;
    managedFileSwap swap ( oneswap, "/tmp/rambrainswap-%d-%d", oneswap );
    cyclicManagedMemory manager ( &swap, oneswap );

    ASSERT_EQ ( mib, swap.getSwapSize() );

    ASSERT_NO_THROW (
        managedPtr<double> ptr1 ( bigdblamount ); // To fill up swap

        ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getUsedMemory() );
        ASSERT_EQ ( 0, manager.getSwappedMemory() );

        managedPtr<double> ptr2 ( smalldblamount ); // Can fit in swap

        ASSERT_EQ ( ( bigdblamount + smalldblamount ) * sizeof ( double ), manager.getUsedMemory() );
        ASSERT_EQ ( ( 0 ) * sizeof ( double ), manager.getSwappedMemory() );

        managedPtr<double> ptr3 ( bigdblamount ); // To fill up memory

        ASSERT_EQ ( (  bigdblamount + smalldblamount ) * sizeof ( double ), manager.getUsedMemory() );
        ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getSwappedMemory() );

        managedPtr<double> ptr4 ( smalldblamount ); // Can fit in swap

        ASSERT_EQ ( ( bigdblamount + 2 * smalldblamount ) * sizeof ( double ), manager.getUsedMemory() );
        ASSERT_EQ ( ( bigdblamount ) * sizeof ( double ), manager.getSwappedMemory() );

        managedPtr<double> ptr5 ( 2 * smalldblamount ); // Can fit in remaining memory

        ASSERT_EQ ( ( bigdblamount + 2 * smalldblamount ) * sizeof ( double ), manager.getUsedMemory() );
        ASSERT_EQ ( ( bigdblamount + 2 * smalldblamount ) * sizeof ( double ), manager.getSwappedMemory() );

        ASSERT_EQ ( MEM_SWAPPED, ptr1.chunk->status );
        ASSERT_EQ ( MEM_SWAPPED, ptr2.chunk->status );
        ASSERT_EQ ( MEM_ALLOCATED, ptr3.chunk->status );
        ASSERT_EQ ( MEM_SWAPPED, ptr4.chunk->status );
        ASSERT_EQ ( MEM_ALLOCATED, ptr5.chunk->status );
    );
}

/**
 * @test Implements a simple matrix transposition that has to be swapping to check for data integrity
 */
TEST ( managedFileSwap, Integration_MatrixTranspose )
{
    const unsigned int size = 1000;
    const unsigned int memlines = 10;
    const unsigned int mem = size * sizeof ( double ) *  memlines;
    const unsigned int swapmem = size * size * sizeof ( double ) * 2;

    managedFileSwap swap ( swapmem, "/tmp/rambrainswap-%d-%d" );
    cyclicManagedMemory manager ( &swap, mem );

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

    // Check result
    for ( unsigned int i = 0; i < size; ++i ) {
        adhereTo<double> rowloc ( *rows[i] );
        const double *rowdbl =  rowloc;
        for ( unsigned int j = 0; j < size; ++j ) {
            ASSERT_EQ ( j * size + i, rowdbl[j] );
        }
    }

    // Delete
    for ( unsigned int i = 0; i < size; ++i ) {
        delete rows[i];
    }
}

RESTORE_WARNINGS;

#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"
#include "managedPtr.h"
#include <gtest/gtest.h>
#include <sys/stat.h>

TEST ( managedFileSwap, Unit_SwapAllocation )
{
    unsigned int oneswap = 1024 * 1024 * 16;
    unsigned int totalswap = 16 * oneswap;
    managedFileSwap swap ( totalswap, "/tmp/membrainswap-%d", oneswap );
    for ( int n = 0; n < 16; n++ ) {
        char fname[70];
        snprintf ( fname, 70, "/tmp/membrainswap-%d", n );
        struct stat mstat;
        stat ( fname, &mstat );
        ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
        ASSERT_EQ ( 0, mstat.st_size );

    }
    cyclicManagedMemory manager ( &swap, 1024 * 10 );
    {
        managedPtr<double> testarr ( 1024 ); //80% RAM full.
        for ( int n = 0; n < 16; n++ ) {
            char fname[70];
            snprintf ( fname, 70, "/tmp/membrainswap-%d", n );
            struct stat mstat;
            stat ( fname, &mstat );
            ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
            ASSERT_EQ ( 0, mstat.st_size );

        }
        managedPtr<double> testarr2 ( 1024 ); //80% RAM full., swap out occurs.
        struct stat first;
        stat ( "/tmp/membrainswap-0", &first );
        ASSERT_TRUE ( S_ISREG ( first.st_mode ) );
        ASSERT_EQ ( 1024 * 1024, first.st_size ); // should be 1/16th pageFileSize
        for ( int n = 1; n < 16; n++ ) {
            char fname[70];
            snprintf ( fname, 70, "/tmp/membrainswap-%d", n );
            struct stat mstat;
            stat ( fname, &mstat );
            ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
            ASSERT_EQ ( 0, mstat.st_size );

        }
        managedPtr<double> testarr3 ( 1024 );
        ASSERT_EQ ( 8 * 1024 * 2, manager.getSwappedMemory() );
        stat ( "/tmp/membrainswap-0", &first );
        ASSERT_TRUE ( S_ISREG ( first.st_mode ) );
        ASSERT_EQ ( 1024 * 1024, first.st_size ); //Size should not change.
        for ( int n = 1; n < 16; n++ ) {
            char fname[70];
            snprintf ( fname, 70, "/tmp/membrainswap-%d", n );
            struct stat mstat;
            stat ( fname, &mstat );
            ASSERT_TRUE ( S_ISREG ( mstat.st_mode ) );
            ASSERT_EQ ( 0, mstat.st_size );

        }
    }
    ASSERT_EQ ( 0, manager.getSwappedMemory() ); //Deallocated all pointers
    ASSERT_EQ ( 0, swap.getUsedSwap() );


    //Start to fill up swap with chunks that are below window size:
    // Wsize = 1MB , 10KiB Objects in RAM, 100 fit exactly in one swapFile.
    //let us have objects that fill RAM completely:
    const unsigned int tenkbdoublearrsize = 1024 / 8 * 10;
    const unsigned int doublearrsize = tenkbdoublearrsize * sizeof ( double );
    managedPtr<double> *ptarr[1641];
    for ( unsigned int n = 0; n < 1639; n++ ) { // Create just as much as fit into RAM+1 swapfile
        ptarr[n] = new managedPtr<double> ( tenkbdoublearrsize );
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


#ifdef SWAPSTATS
    manager.printSwapstats();
#endif


};
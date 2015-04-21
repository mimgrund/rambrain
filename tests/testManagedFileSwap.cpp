#include "cyclicManagedMemory.h"
#include "managedFileSwap.h"
#include "managedPtr.h"
#include <gtest/gtest.h>
#include <sys/stat.h>

TEST ( managedFileSwap, Unit_SimpleSwapping )
{
    const unsigned int memsize = 15 * sizeof ( double );
    const unsigned int swapsize = 10 * memsize;

    ASSERT_NO_THROW (
        managedFileSwap swap ( swapsize, "/tmp/membrainswap-%d" );
        cyclicManagedMemory manager ( &swap, memsize );

        managedPtr<double> ptr1 ( 10 );
        managedPtr<double> ptr2 ( 10 );
    );
}

TEST ( managedFileSwap, Unit_SwapSize )
{
    const unsigned int dblamount = 100;
    const unsigned int dblsize = dblamount * sizeof ( double );
    const unsigned int swapmem = dblsize * 10;
    const unsigned int memsize = dblsize * 1.5;
    const unsigned int onemb = 1048576;
    managedFileSwap swap ( swapmem, "/tmp/membrainswap-%d" );
    cyclicManagedMemory manager ( &swap, memsize );

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    managedPtr<double> *ptr1 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    ptr1->setUse();

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    ptr1->unsetUse();

    managedPtr<double> *ptr2 = new managedPtr<double> ( dblamount );

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    ptr2->setUse();

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    ptr2->unsetUse();

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( dblsize, swap.getUsedSwap() );

    delete ptr1;

    ASSERT_EQ ( onemb, swap.getSwapSize() );
    ASSERT_EQ ( 0u, swap.getUsedSwap() );

    delete ptr2;
}


TEST ( managedFileSwap, Integration_RandomAccess )
{
    global_bytesize oneswap = 1024 * 1024 * ( global_bytesize ) 160;
    global_bytesize totalswap = 16 * oneswap;
    managedFileSwap swap ( totalswap, "./membrainswap-%d", oneswap );
    cyclicManagedMemory manager ( &swap, oneswap );

    /*ASSERT_EQ ( 0, manager.getSwappedMemory() ); //Deallocated all pointers
    ASSERT_EQ ( 0, swap.getUsedSwap() );
    ASSERT_EQ ( 16, swap.all_space.size() );
    ASSERT_EQ ( 16, swap.free_space.size() );*/
    infomsgf ( "%ld total swap in %ld swapfiles", swap.getSwapSize(), swap.all_space.size() );

    global_bytesize obj_size = 102400 * sizeof ( double );
    global_bytesize obj_no = totalswap / obj_size * 2;


    managedPtr<double> **objmask = ( managedPtr<double> ** ) malloc ( sizeof ( managedPtr<double> * ) *obj_no );
    for ( unsigned int n = 0; n < obj_no; ++n ) {
        objmask[n] = NULL;
    }
    for ( unsigned int n = 0; n < 10 * obj_no; ++n ) {
        global_bytesize no = ( ( double ) rand() / RAND_MAX ) * obj_no;

        if ( objmask[no] == NULL ) {
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
    ASSERT_EQ ( 16, swap.all_space.size() );
    ASSERT_EQ ( 16, swap.free_space.size() );

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


};

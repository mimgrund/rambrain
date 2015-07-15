#include <iostream>
#include "managedPtr.h"
#include "dummyManagedMemory.h"
#include "managedDummySwap.h"
#include "membrainconfig.h"

using namespace std;
using namespace membrain;

int main ( int argc, char **argv )
{
    int ret = 0;
    cout << "Starting check if binary specific config is read and used properly" << endl;

    membrainglobals::config.setCustomConfigPath ( "../tests/testConfig.conf" );
    membrainglobals::config.reinit ( true );

    const configuration &config = membrainglobals::config.getConfig();
    managedMemory *man = managedMemory::defaultManager;

    if ( config.memoryManager != "dummyManagedMemory" || reinterpret_cast<dummyManagedMemory *> ( man ) == NULL ) {
        cerr << "Manager is wrong!" << endl;
        ++ ret;
    }

    /// @todo check if correct swap is in place
    if ( config.swap != "managedDummySwap" ) {
        cerr << "Swap is wrong!" << endl;
        ++ ret;
    }

    if ( config.memory != 100 || man->getMemoryLimit() != 100ul ) {
        cerr << "Memory limit is wrong!" << endl;
        ++ ret;
    }

    /// @todo check if swap is correct size
    if ( config.swapMemory != 1000 ) {
        cerr << "Swap limit is wrong!" << endl;
        ++ ret;
    }

    /// @todo also check this in system
    if ( config.swapfiles != "membrainswapconfigtest-%d-%d" ) {
        cerr << "Swap files are named incorrectly!" << endl;
        ++ ret;
    }

    managedPtr<double> data1 ( 10, 2.0 );
    managedPtr<double> data2 ( 10, 4.0 );

    {
        ADHERETOLOC ( double, data1, d );
        for ( int i = 0; i < 10; ++i ) {
            if ( d[i] != 2.0 ) {
                cerr << "Swapped in data is wrong at index " << i << endl;
                ++ ret;
            }
        }
    }

    cout << "Done, " << ret << " errors occured" << endl;

    return ret;
}

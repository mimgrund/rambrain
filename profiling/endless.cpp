#include <iostream>
#include <cmath>
#include <cstdlib>

#include <rambrain/rambrainDefinitionsHeader.h>
#include <rambrain/managedPtr.h>
#include <rambrain/rambrainconfig.h>
#include <rambrain/cyclicManagedMemory.h>

using namespace std;
using namespace rambrain;

int main(void)
{
    // Adjust available memory and swap
    global_bytesize oneswap = 1024 * 1024 * ( global_bytesize ) 16;
    global_bytesize totalswap = 32 * oneswap;

    rambrainglobals::config.resizeMemory ( oneswap );
    rambrainglobals::config.resizeSwap ( totalswap );

    srand48(time(NULL));

    // Alocate structures
    global_bytesize obj_size = 102400 * sizeof ( double );
    global_bytesize obj_no = totalswap / obj_size * 2;

    managedPtr<double> **objmask = ( managedPtr<double> ** ) malloc ( sizeof ( managedPtr<double> * ) *obj_no );
    for ( unsigned int n = 0; n < obj_no; ++n )
    {
        objmask[n] = NULL;
    }

    // Do something with the data
    int i = 99E3;
    while ( true )
    {
       // We can profile what happens programatically for example like this:
       if (i == 1E5) 
         {
            cyclicManagedMemory* manager = static_cast<cyclicManagedMemory*>(managedMemory::defaultManager);
            cout << "Checking cycle: " << (manager->checkCycle() ? "Checked!" : "Failure!") << endl;
            cout << "Printing cycle...." << endl;
            manager->printCycle();
            cout << "Printing swapstats:" << endl;
            managedMemory::defaultManager->printSwapstats();
            i = 0;
         }
       
        global_bytesize no = drand48() * obj_no;
        if ( objmask[no] == NULL )
        {
            // Create new data at spot no
            unsigned int varsize = ( drand48() + 0.5 ) * 102400;
           if ( ( varsize + 102400 * 2. ) *sizeof ( double ) > managedMemory::defaultManager->getFreeSwapMemory() )
            {
                continue;
            }

            objmask[no] = new managedPtr<double> ( varsize );
            {
                adhereTo<double> objoloc ( *objmask[no] );
                double *darr =  objoloc;
                for ( unsigned int k = 0; k < varsize; k++ )
                {
                    darr[k] = k + varsize;
                }
            }
        } else {
            // Delete data at spot no
            delete objmask[no];
            objmask[no] = NULL;
        }
       ++ i;
    }

    // Cleanup, won't be reached since infinite loop
    for ( unsigned int n = 0; n < obj_no; ++n )
    {
        if ( objmask[n] != NULL )
        {
            delete objmask[n];
        }
    }
    free ( objmask );

    return 0;
}

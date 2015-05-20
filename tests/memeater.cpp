#include <sys/mman.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

using namespace std;

int main ( int argc, char **argv )
{
    if ( argc < 2 ) {
        printf ( "Usage: ./membrain-memeater <MB to leave free>\n" );
        return -1;
    }

    ifstream meminfo ( "/proc/meminfo", ifstream::in );
    char line[1024];
    for ( int c = 0; c < 3; ++c ) {
        meminfo.getline ( line, 1024 );
    }

    printf ( line );
    long mbytes_avail;

    char *begin = NULL, *end = NULL;
    char *pos = line;
    while ( !end ) {
        if ( *pos <= '9' && *pos >= '0' ) {
            if ( begin == NULL ) {
                begin = pos;
            }
        } else {
            if ( begin != NULL ) {
                end = pos;
            }
        }
        ++pos;
    }
    * ( end + 1 ) = 0x00;
    mbytes_avail = atol ( begin ) / 1024;
    long mbytes_holdfree = atoi ( argv[1] );
    long mb_alloc = mbytes_avail - mbytes_holdfree;

    printf ( "\nWe detected %lu free mBytes\nAllocating %lu\n", mbytes_avail, mb_alloc );
    if ( mb_alloc <= 0 ) {
        printf ( "You don't need to nomnom, we're already fed up." );
        return -1;
    }


    unsigned int chunkSize = 1024 * 1024;

    void *malloced[mb_alloc];
    for ( int c = 0; c < mb_alloc; ++c ) {
        malloced[c] = malloc ( chunkSize );
        for ( unsigned int d = 0; d < chunkSize; ++d ) {
            ( ( char * ) malloced[c] ) [d] = 0x07;
        }
    }
    mlockall ( MCL_CURRENT );
    printf ( "Oink, **burpp**" );
    int bah ;
    cin >> bah;
    munlockall();
}



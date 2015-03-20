#include "managedFileSwap.h"
#include "common.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "exceptions.h"
managedFileSwap::managedFileSwap ( unsigned int size, const char *filemask, unsigned int oneFileBytes ) : managedSwap ( size )
{
    pageSize = sysconf ( _SC_PAGE_SIZE );

    //calculate page File number:
    oneFileBytes *= 1024; // we calc bytewise
    unsigned int padding = oneFileBytes % pageSize;
    if ( padding != 0 ) {
        warnmsgf ( "requested single swap filesize is not a multiple of pageSize.\n\t %u Bytes left over.", padding );
    }

    oneFileBytes += pageSize - padding;
    if ( size % oneFileBytes != 0 ) {
        pageFileNumber = size / oneFileBytes + 1;
    } else {
        pageFileNumber = size / oneFileBytes;
    }

    //initialize inherited members:
    swapSize = pageFileNumber * oneFileBytes;
    swapUsed = 0;

    //copy filemask:
    this->filemask = malloc ( sizeof ( char ) * ( strlen ( filemask ) + 1 ) );
    strcpy ( this->filemask, filemask );

    if ( !openSwapFiles() ) {
        throw memoryException ( "Could not create swap files" );
    }

    //Initialize swapmalloc:
    free_entry = determineOffsets ( 0, swapSize );

}

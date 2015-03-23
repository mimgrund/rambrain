#include "managedFileSwap.h"
#include "common.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "exceptions.h"
managedFileSwap::managedFileSwap ( unsigned int size, const char *filemask, unsigned int oneFile ) : managedSwap ( size )
{


    //calculate page File number:
    unsigned int padding = oneFile % pageSize;
    if ( padding != 0 ) {
        warnmsgf ( "requested single swap filesize is not a multiple of pageSize.\n\t %u Bytes left over.", padding );
    }

    oneFile += pageSize - padding;
    pageFileSize = oneFile;
    if ( size % oneFile != 0 ) {
        pageFileNumber = size / oneFile + 1;
    } else {
        pageFileNumber = size / oneFile;
    }

    //initialize inherited members:
    swapSize = pageFileNumber * oneFile;
    swapUsed = 0;

    //copy filemask:
    this->filemask = ( char * ) malloc ( sizeof ( char ) * ( strlen ( filemask ) + 1 ) );
    strcpy ( ( char * ) this->filemask, filemask );

    swapFiles = NULL;
    if ( !openSwapFiles() ) {
        throw memoryException ( "Could not create swap files" );
    }

    //Initialize swapmalloc:
    free_entry = NULL;
    pageFileLocation *old = NULL;
    for ( unsigned int n = 0; n < pageFileNumber; n++ ) {
        pageFileLocation *pfloc = new pageFileLocation;
        if ( !free_entry ) {
            free_entry = pfloc;
        }
        if ( old ) {
            old->next = pfloc;
        }
        pfloc->file = n;
        pfloc->offset = 0;
        pfloc->size = pageSize;
        pfloc->freetracker = NULL;
        pfloc->next = NULL;
        pfloc->status = PAGE_FREE;
        old = pfloc;
    }

    //Initialize Windows:
    unsigned int ws_ratio = .1 * pageFileSize; //TODO: unhardcode that buddy
    unsigned int ws_max = 100 * 1024 * 1024; //TODO: unhardcode that buddy
    windowNumber = 10;
    windowSize = min ( ws_max, ws_ratio );

    //Initialize one swap-window into first swap file:
    windows = new pageFileWindow *[windowNumber];
    for ( unsigned int n = 0; n < windowNumber; ++n ) {
        windows[n] = 0;
    }
    old->size = windowSize;
    windows[0] = new pageFileWindow ( *old, *this );

}

managedFileSwap::~managedFileSwap()
{
    if ( windows ) {
        for ( int n = 0; n < windowNumber; ++n )
            if ( windows[n] ) {
                delete windows[n];
            }
        delete windows;
        windows = NULL;
    };
    closeSwapFiles();
    free ( ( void * ) filemask );
}


void managedFileSwap::closeSwapFiles()
{
    if ( swapFiles ) {
        for ( unsigned int n = 0; n < pageFileNumber; ++n ) {
            fclose ( swapFiles[n] );
        }
        delete swapFiles;
        swapFiles == NULL;
    }

}


bool managedFileSwap::openSwapFiles()
{
    if ( swapFiles ) {
        throw memoryException ( "Swap files already opened. Close first" );
        return false;
    }
    swapFiles = new FILE *[pageFileNumber];
    for ( unsigned int n = 0; n < pageFileNumber; ++n ) {
        char fname[1024];
        snprintf ( fname, 1024, filemask, n );
        swapFiles[n] = fopen ( fname, "r+" );
        if ( !swapFiles[n] ) {
            throw memoryException ( "Could not open swap file." );
            return false;
        }
        ftruncate ( fileno ( swapFiles[n] ), pageFileSize );
    }
    return true;
}

const unsigned int managedFileSwap::pageSize = sysconf ( _SC_PAGE_SIZE );
//Actual interface:
void managedFileSwap::swapDelete ( managedMemoryChunk *chunk )
{

}
bool managedFileSwap::swapIn ( managedMemoryChunk *chunk )
{

}
unsigned int managedFileSwap::swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks )
{

}

bool managedFileSwap::swapOut ( managedMemoryChunk *chunk )
{
}
unsigned int managedFileSwap::swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks )
{

}





//Page File Window Class:

pageFileWindow::pageFileWindow ( const pageFileLocation &location, managedFileSwap &swap )
{
    unsigned int pm_padding = location.offset % managedFileSwap::pageSize;
    unsigned int pm_end_padding = ( location.offset + location.size ) % managedFileSwap::pageSize;
    offset = location.offset;
    length = location.size;
    file = location.file;
    if ( pm_padding > 0 ) {
        offset -= pm_padding;
        length += pm_padding;
    }
    length += pm_end_padding;
    const unsigned int fd = fileno ( swap.swapFiles[location.file] );
    buf = mmap ( NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset );
    if ( buf == MAP_FAILED ) {
        throw memoryException ( "memory map failed!" );
    }

}

pageFileWindow::~pageFileWindow()
{
    munmap ( buf, length );
}

void pageFileWindow::triggerSync ( bool async )
{
    msync ( buf, length, async ? MS_ASYNC : MS_SYNC );
}


void *pageFileWindow::getMem ( const pageFileLocation &loc )
{
    if ( loc.file != file ) {
        return NULL;
    }
    if ( loc.offset < offset ) {
        return NULL;
    }
    if ( loc.offset + loc.size > offset + length ) {
        return NULL;
    }
    return ( char * ) buf + ( loc.offset - offset );

}



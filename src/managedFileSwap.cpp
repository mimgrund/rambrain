#include "managedFileSwap.h"
#include "common.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/stat.h>
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
            old->next_glob = old->next_prt = pfloc;
        }
        pfloc->file = n;
        pfloc->offset = 0;
        pfloc->size = pageSize;
        pfloc->next_glob = NULL;
        pfloc->next_prt = NULL;
        pfloc->prev_glob = old;
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
    char *test = ( char * ) windows[0]->getMem ( *old );
    for ( int n = 0; n < windowSize; ++n ) {
        test[n] = n % 256;
    }

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
        swapFiles[n] = fopen ( fname, "w+" );
        if ( !swapFiles[n] ) {
            throw memoryException ( "Could not open swap file." );
            return false;
        }
    }
    return true;
}

const unsigned int managedFileSwap::pageSize = sysconf ( _SC_PAGE_SIZE );

pageFileLocation *managedFileSwap::pfmalloc ( unsigned int size )
{
    //TODO: This may be rather stupid at the moment

    /*Priority: -Use first free chunk that fits completely
     *          -Distribute over free locations
                Later on/TODO: look for read-in memory that can be overwritten*/
    pageFileLocation *cur = free_entry;
    pageFileLocation *prev = NULL;
    while ( cur ) {
        if ( cur->size >= size ) {
            break;
        }
        prev = cur;
        cur = cur->next;

    }
    pageFileLocation *res = NULL;
    if ( cur ) { //Found enough space for struct
        //Unlink free space:
        pageFileLocation *prev_glob = cur->prev_glob;
        pageFileLocation *next_glob = allocInFree ( cur, prev );


        res = new pageFileLocation;
        res->file = cur->file;
        res->offset = cur->offset;
        res->size = size;
        res->status = PAGE_END;
        res->prev_glob = prev_glob;
        res->next_glob = next_glob;
        res->next_prt = NULL;

    } else { //Let's find enough space for us
        unsigned int totsize = 0;

        cur = free_entry;
        while ( cur ) {
            totsize += cur->size;
            if ( totsize >= size ) {
                break;
            }
            prev = cur;
            cur = cur->next_prt;
        }
        if ( cur ) { //We found enough memory to comply.
            //Hang this memory out of free chunks:

            pageFileLocation *cur2 = free_entry;
            do {


                cur2 = cur2->next;
            } while ( cur2 != cur->next );

        } else {
            return throw memoryException ( "Out of swap memory" );
        }

    }

}

pageFileLocation *managedFileSwap::allocInFree ( pageFileLocation *freeChunk, pageFileLocation *prevChunk, unsigned int size )
{
    //Is there still free space in chunk?
    unsigned int newfreespace = freeChunk->size - size;
    pageFileLocation *next;
    if ( newfreespace > sizeof ( pageFileLocation ) ) { //Raw criterion for mem-wastage
        freeChunk->size = newfreespace;
        freeChunk->offset += size;//no swapChunks can span over swapfile border.
        next = freeChunk;
    } else { //Take out this free chunk as a whole:
        next = freeChunk->next_prt;
        delete freeChunk;
    }
    if ( prevChunk ) {
        prevChunk->next_prt = freeChunk;
    } else {
        free_entry = freeChunk;
    }
    return next;
}



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
    //check whether swapfile is big enough, if not, rescale:
    struct stat stats;
    fstat ( fd, &stats );
    unsigned int destsize = offset + length;
    if ( stats.st_size < destsize ) {

        if ( ftruncate ( fd, destsize ) == -1 ) {
            throw memoryException ( "could not resize swap file" );
        }
    }


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



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
    if ( oneFile == 0 ) { // Layout this on your own:

        unsigned int gig = 1024 * 1024 * 1024;
        unsigned int myg = size / 16;
        unsigned int mib = 1024 * 1024;
        oneFile = min ( gig, myg );
        oneFile = max ( mib, oneFile );
    }
    //calculate page File number:
    unsigned int padding = oneFile % pageSize;
    if ( padding != 0 ) {
        warnmsgf ( "requested single swap filesize is not a multiple of pageSize.\n\t %u Bytes left over.", padding );
        oneFile += pageSize - padding;
    }
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
    for ( unsigned int n = 0; n < pageFileNumber; n++ ) {
        pageFileLocation *pfloc = new pageFileLocation;
        pfloc->file = n;
        pfloc->offset = 0;
        pfloc->size = pageFileSize;
        pfloc->glob_off_next = NULL; // We may use 0 for invalid, as the first one is never the next.
        pfloc->status = PAGE_FREE;
        free_space[n * pageFileSize] = pfloc;
        all_space[n * pageFileSize] = pfloc;
    }

    //Initialize Windows:
    unsigned int ws_ratio = pageFileSize / 16; //TODO: unhardcode that buddy
    unsigned int ws_max = 128 * 1024 * 1024; //TODO: unhardcode that buddy
    windowNumber = 10;
    windowSize = min ( ws_max, ws_ratio );
    windowSize += ( windowSize % pageSize ) > 0 ? ( pageSize - ( windowSize % pageSize ) ) : 0;
    if ( pageFileSize % windowSize != 0 ) {
        warnmsg ( "requested single swap filesize is not a multiple of pageSize*16 or 128MiB" );
    }

    //Initialize one swap-window into first swap file:
    windows = ( pageFileWindow ** ) malloc ( sizeof ( pageFileWindow * ) *windowNumber );
    for ( unsigned int n = 0; n < windowNumber; ++n ) {
        windows[n] = 0;
    }
}

managedFileSwap::~managedFileSwap()
{
    if ( windows ) {
        for ( unsigned int n = 0; n < windowNumber; ++n )
            if ( windows[n] ) {
                delete windows[n];
            }
        free ( windows );
    };
    closeSwapFiles();
    free ( ( void * ) filemask );
    if ( all_space.size() > 0 ) {
        std::map<unsigned int, pageFileLocation *>::iterator it = all_space.begin();
        do {
            delete it->second;
        } while ( ++it != all_space.end() );
    }
}


void managedFileSwap::closeSwapFiles()
{
    if ( swapFiles ) {
        for ( unsigned int n = 0; n < pageFileNumber; ++n ) {
            fclose ( swapFiles[n] );
        }
        free ( swapFiles );
    }

}


bool managedFileSwap::openSwapFiles()
{
    if ( swapFiles ) {
        throw memoryException ( "Swap files already opened. Close first" );
        return false;
    }
    swapFiles = ( FILE ** ) malloc ( sizeof ( FILE * ) *pageFileNumber );
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
    std::map<unsigned int, pageFileLocation *>::iterator it = free_space.begin();
    pageFileLocation *found = NULL;
    do {
        if ( it->second->size >= size ) {
            found = it->second;
            break;
        }
    } while ( ++it != free_space.end() );
    pageFileLocation *res = NULL;
    pageFileLocation *former = NULL;
    if ( found ) {
        res = allocInFree ( found, size );
        res->status = PAGE_END;//Don't forget to set the status of the allocated memory.
    } else { //We need to write out the data in parts.


        //check for enough space:
        unsigned int total_space = 0;
        it = free_space.begin();
        while ( true ) {
            total_space += it->second->size;
            if ( total_space >= size ) {
                break;
            }
        }
        while ( ++it != free_space.end() );
        std::map<unsigned int, pageFileLocation *>::iterator it2 = free_space.begin();


        if ( total_space >= size ) { //We can concat enough free chunks to satisfy memory requirements
            while ( true ) {
                unsigned int avail_space = it2->second->size;
                unsigned int alloc_here = min ( avail_space, total_space );
                pageFileLocation *neu = allocInFree ( it2->second, alloc_here );
                total_space -= alloc_here;
                neu->status = total_space == 0 ? PAGE_END : PAGE_PART;
                if ( !res ) {
                    res = neu;
                }
                if ( former ) {
                    former->glob_off_next = neu;
                }
                former = res;
                if ( it2 == it ) {
                    break;
                } else {
                    it++;
                }
            };

        } else {
            throw memoryException ( "Out of swap space" );
        }

    }
    return res;
}

pageFileLocation *managedFileSwap::allocInFree ( pageFileLocation *freeChunk, unsigned int size )
{
    //Hook out the block of free space:
    global_offset formerfree_off = determineGlobalOffset ( *freeChunk );
    free_space.erase ( formerfree_off );
    //We want to allocate a new chunk or use the chunk at hand.
    if ( freeChunk->size - size < sizeof ( pageFileLocation ) ) { //Memory to manage free space exceeds free space (actually more than)
        //Thus, use the free chunk for your data.
        freeChunk->size = size;
        return freeChunk;
    } else {
        pageFileLocation *neu = new pageFileLocation ( *freeChunk );
        freeChunk->offset += size;
        freeChunk->size -= size;
        freeChunk->glob_off_next = NULL;
        global_offset newfreeloc = determineGlobalOffset ( *freeChunk );
        free_space[newfreeloc] = freeChunk;
        neu->size = size;
        all_space[newfreeloc] = freeChunk; //inserts.
        all_space[formerfree_off] = neu;//overwrites.
        return neu;

    }

}

void managedFileSwap::pffree ( pageFileLocation *pagePtr )
{
    pageFileLocation *free_start = pagePtr;
    bool endIsReached = false;
    do { //We possibly need multiple frees.
        pageFileLocation *next = pagePtr->glob_off_next;
        endIsReached = ( pagePtr->status == PAGE_END );
        global_offset goff = determineGlobalOffset ( *pagePtr );
        auto it = all_space.find ( goff );
        //Check if we have free space before us to merge with:
        if ( pagePtr->offset != 0 && it != all_space.begin() )
            if ( ( --it )->second->status == PAGE_FREE ) {
                //Merge previous free space with this chunk
                pageFileLocation *prev = it->second;
                prev->size += pagePtr->size;
                delete pagePtr;
                all_space.erase ( goff );
                pagePtr = prev;
            }
        //Check if we have free space after us to merge with;
        goff = determineGlobalOffset ( *pagePtr );
        it = all_space.find ( goff );
        ++it;
        if ( it != all_space.end() && it->second->offset != 0 && it->second->status == PAGE_FREE ) {
            //We may merge the pageFileLocation after ourselve:
            global_offset gofffree = determineGlobalOffset ( * ( it->second ) );
            pagePtr->size += it->second->size;
            //The second one may go completely:
            delete it->second;
            free_space.erase ( gofffree );
            all_space.erase ( gofffree );

        }

        //We are left with our free chunk, lets mark it free (possibly redundant.)
        pagePtr->status = PAGE_FREE;
        free_space[goff] = pagePtr;
        pagePtr = next;

    } while ( !endIsReached );

}


//Actual interface:
void managedFileSwap::swapDelete ( managedMemoryChunk *chunk )
{
    if ( chunk->swapBuf ) { //Must not be swapped, as read-only access should lead to keeping the swapped out locs for the moment.
        pageFileLocation *loc = ( pageFileLocation * ) chunk->swapBuf;
        pffree ( loc );
    }
    swapUsed -= chunk->size;
}

bool managedFileSwap::swapIn ( managedMemoryChunk *chunk )
{
    void *buf = malloc ( chunk->size );
    if ( !chunk->swapBuf ) {
        return false;
    }
    if ( buf ) {
        chunk->locPtr = buf;
        copyMem ( chunk->locPtr, * ( ( pageFileLocation * ) chunk->swapBuf ) );
        pffree ( ( pageFileLocation * ) chunk->swapBuf );
        chunk->swapBuf = NULL; // Not strictly required
        chunk->status = MEM_ALLOCATED;
        swapUsed -= chunk->size;
        return true;
    } else {
        return false;
    }
}

unsigned int managedFileSwap::swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    unsigned int n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += ( swapIn ( chunklist[n] ) ? 1 : 0 );
    }
    return n_swapped;
}

bool managedFileSwap::swapOut ( managedMemoryChunk *chunk )
{
    if ( chunk->size + swapUsed > swapSize ) {
        return false;
    }

    if ( chunk->swapBuf ) { //We already have a position to store to! (happens when read-only was triggered)
        //TODO: implement swapOUt if we already hold a memory copy.

    } else {
        pageFileLocation *newAlloced = pfmalloc ( chunk->size );
        if ( newAlloced ) {
            chunk->swapBuf = newAlloced;
            copyMem ( *newAlloced, chunk->locPtr );
            free ( chunk->locPtr );//TODO: move allocation and free to managedMemory...
            chunk->locPtr = NULL; // not strictly required.
            chunk->status = MEM_SWAPPED;
            swapUsed += chunk->size;
            return true;
        } else {
            return false;
        }
    }
    return false;

}
unsigned int managedFileSwap::swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    unsigned int n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += ( swapOut ( chunklist[n] ) ? 1 : 0 );
    }
    return n_swapped;
}


global_offset managedFileSwap::determineGlobalOffset ( const pageFileLocation &ref )
{
    return ref.file * pageFileSize + ref.offset;
}
pageFileLocation managedFileSwap::determinePFLoc ( global_offset g_offset, unsigned int length )
{
    pageFileLocation pfLoc;
    pfLoc.size = length;
    pfLoc.file = g_offset / pageFileSize;
    pfLoc.offset = g_offset - pfLoc.file * pageFileSize;
    pfLoc.glob_off_next = NULL;
    pfLoc.status = PAGE_UNKNOWN_STATE;
    return pfLoc;
}



pageFileWindow *managedFileSwap::getWindowTo ( const pageFileLocation &loc )
{
    unsigned int w;
    for ( w = 0; w < windowNumber; ++w ) {
        if ( !windows[w] ) {
            break;
        }
        if ( windows[w]->isInWindow ( loc ) ) {
            return windows[w];
        }

    }
    if ( w == windowNumber ) {
        //We have to throw out a window
        //TODO: More clever selection strategy than:
        w = ( lastCreatedWindow + 1 ) % windowNumber;
        delete windows[w];
    }
    windows[w] = new pageFileWindow ( loc, *this );
    lastCreatedWindow = w;
    return windows[w];
}

void *managedFileSwap::getMem ( const pageFileLocation &loc, unsigned int &size )
{
    pageFileWindow *window = getWindowTo ( loc );
    return window->getMem ( loc, size );

}


void managedFileSwap::copyMem ( const pageFileLocation &ref, void *ramBuf )
{
    const pageFileLocation *cur = &ref;
    char *cramBuf = ( char * ) ramBuf;
    unsigned int offset = 0;
    while ( true ) { //Sift through all pageChunks that have to be read
        global_offset g_off = determineGlobalOffset ( *cur );

        unsigned int pageChunkSize = cur->size;
        while ( pageChunkSize > 0 ) { //Sift through potential window change
            pageFileLocation loc = determinePFLoc ( g_off, pageChunkSize );
            unsigned int pagedIn = pageChunkSize;
            void *pageFileBuf = getMem ( loc, pagedIn );
            memcpy ( pageFileBuf, cramBuf + offset, pagedIn );
            pageChunkSize -= pagedIn;
            offset += pagedIn;
            g_off += pagedIn;
        }

        if ( cur->status == PAGE_END ) {//I have completely written this pageChunk.
            break;
        }
        cur = cur->glob_off_next;
    };
}

void managedFileSwap::copyMem ( void *ramBuf, const pageFileLocation &ref )
{
    const pageFileLocation *cur = &ref;
    char *cramBuf = ( char * ) ramBuf;
    unsigned int offset = 0;
    while ( true ) { //Sift through all pageChunks that have to be read
        global_offset g_off = determineGlobalOffset ( *cur );

        unsigned int pageChunkSize = cur->size;
        while ( pageChunkSize > 0 ) { //Sift through potential window change
            pageFileLocation loc = determinePFLoc ( g_off, pageChunkSize );
            unsigned int pagedIn = pageChunkSize;
            void *pageFileBuf = getMem ( loc, pagedIn );
            memcpy ( cramBuf + offset, pageFileBuf, pagedIn );
            pageChunkSize -= pagedIn;
            offset += pagedIn;
            g_off += pagedIn;
        }
        if ( cur->status == PAGE_END ) {
            break;
        }
        cur = cur->glob_off_next;
    };
}


//Page File Window Class:

pageFileWindow::pageFileWindow ( const pageFileLocation &location, managedFileSwap &swap )
{
    //We use fixed size windows as everything else will be very complicated when determining to which windows to write to.
    offset = location.offset - location.offset % swap.windowSize; //local offset in file

    length = swap.windowSize; //window length.
    file = location.file; //which file.

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


void *pageFileWindow::getMem ( const pageFileLocation &loc, unsigned int &size )
{
    if ( loc.file != file ) {
        return NULL;
    }
    if ( loc.offset < offset ) {
        return NULL;
    }
    if ( loc.offset > offset + length ) {
        return NULL;
    }
    unsigned int windowBdryDistance = length - loc.offset % length;
    size = min ( windowBdryDistance, size );

    return ( char * ) buf + ( loc.offset - offset );


}

bool pageFileWindow::isInWindow ( const pageFileLocation &location, unsigned int &offset_start, unsigned int &offset_end )
{
    if ( location.file != file ) {
        return false;
    }
    bool startIn = false;
    bool endIn = false;
    if ( location.offset >= offset && location.offset < offset + length ) {
        startIn = true;
        offset_start = location.offset;
    } else {
        offset_start = 0;
    }
    if ( location.offset + location.size >= offset && location.offset + location.size < offset + length ) {
        endIn = true;
        offset_end = location.offset + location.size;
    } else {
        offset_end = offset + length;
    }
    if ( !startIn && !endIn ) {
        return false;
    }


    return startIn || endIn;

}
bool pageFileWindow::isInWindow ( const pageFileLocation &location, unsigned int &length )
{
    if ( location.file != file ) {
        return false;
    }
    bool startIn = false;
    unsigned int offset_start, offset_end;
    if ( location.offset >= offset && location.offset < offset + length ) {
        startIn = true;
        offset_start = location.offset;
    } else {
        return false;
        offset_start = 0;
    }
    if ( location.offset + location.size >= offset && location.offset + location.size < offset + length ) {
        offset_end = location.offset + location.size;
    } else {
        offset_end = offset + length;
    }
    length = offset_end - offset_start;
    return true;
}
bool pageFileWindow::isInWindow ( const pageFileLocation location )
{
    if ( location.file != file ) {
        return false;
    }
    if ( location.offset >= offset && location.offset < offset + length ) {
        return true;
    }
    return false;


}

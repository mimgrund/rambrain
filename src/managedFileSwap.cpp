#include "managedFileSwap.h"
#include "common.h"
#include <unistd.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include "exceptions.h"
#include "managedMemory.h"
#include <aio.h>
#include <signal.h>
namespace membrain
{
  
managedFileSwap::managedFileSwap ( global_bytesize size, const char *filemask, global_bytesize oneFile ) : managedSwap ( size ), pageSize ( sysconf ( _SC_PAGE_SIZE ) )
{
    
  
    if ( oneFile == 0 ) { // Layout this on your own:

        global_bytesize myg = size / 16;
        oneFile = min ( gig, myg );
        oneFile = max ( mib, oneFile );
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
    swapFree = swapSize;

    //copy filemask:
    this->filemask = ( char * ) malloc ( sizeof ( char ) * ( strlen ( filemask ) + 1 ) );
    strcpy ( ( char * ) this->filemask, filemask );

    swapFiles = NULL;
    if ( !openSwapFiles() ) {
        throw memoryException ( "Could not create swap files" );
    }

    //Initialize swapmalloc:
    for ( unsigned int n = 0; n < pageFileNumber; n++ ) {
        pageFileLocation *pfloc = new pageFileLocation(n,0,pageFileSize);
        free_space[n * pageFileSize] = pfloc;
        all_space[n * pageFileSize] = pfloc;
    }

    //Initialize Windows:
    global_bytesize ws_ratio = pageFileSize / 16; ///\todo unhardcode that buddy
    global_bytesize ws_max = 128 * mib; ///\todo  unhardcode that buddy
    windowNumber = 10;
    windowSize = min ( ws_max, ws_ratio );
    windowSize += ( windowSize % pageSize ) > 0 ? ( pageSize - ( windowSize % pageSize ) ) : 0;
    if ( pageFileSize % windowSize != 0 ) {
        warnmsg ( "requested single swap filesize is not a multiple of pageSize*16 or 128MiB" );
    }

    instance = this;
    signal ( SIGUSR2, managedFileSwap::sigStat );
    
    //Event handler for aio:
      evhandler.sigev_notify = SIGEV_THREAD;
      evhandler     .sigev_signo = 0;
      evhandler.sigev_value.sival_ptr = NULL;
      evhandler.sigev_notify_function = managedFileSwap::staticAsyncIoArrived;
      evhandler.sigev_notify_attributes=0;
      

}

managedFileSwap::~managedFileSwap()
{
    closeSwapFiles();
    free ( ( void * ) filemask );
    if ( all_space.size() > 0 ) {
        std::map<global_offset, pageFileLocation *>::iterator it = all_space.begin();
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
    ///\todo Limit number of open swap file descriptors to something reasonable (<100?)
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

pageFileLocation *managedFileSwap::pfmalloc ( global_bytesize size )
{
    ///\todo This may be rather stupid at the moment

    /*Priority: -Use first free chunk that fits completely
     *          -Distribute over free locations
                look for read-in memory that can be overwritten*/
    std::map<global_offset, pageFileLocation *>::iterator it = free_space.begin();
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
        global_bytesize total_space = 0;
        it = free_space.begin();
        do {
            total_space += it->second->size;
        } while ( total_space < size && ++it != free_space.end() );

        if ( total_space >= size ) { //We can concat enough free chunks to satisfy memory requirements
            it = free_space.begin();
            while ( true ) {
                global_bytesize avail_space = it->second->size;
                global_bytesize alloc_here = min ( avail_space, size );
                auto it2 = it;
                ++it2;
                global_offset nextFreeOffset = it2->first;
                pageFileLocation *neu = allocInFree ( it->second, alloc_here );
                size -= alloc_here;
                neu->status = ( size == 0 ? PAGE_END : PAGE_PART );

                if ( !res ) {
                    res = neu;
                }
                if ( former ) {
                    former->glob_off_next = neu;
                }
                if ( size == 0 ) {
                    break;
                }
                former = neu;
                it = free_space.find ( nextFreeOffset );
            };

        } else {
            throw memoryException ( "Out of swap space" );
        }

    }
    return res;
}

pageFileLocation *managedFileSwap::allocInFree ( pageFileLocation *freeChunk, global_bytesize size )
{
    //Hook out the block of free space:
    global_offset formerfree_off = determineGlobalOffset ( *freeChunk );
    free_space.erase ( formerfree_off );
    //We want to allocate a new chunk or use the chunk at hand.
    if ( freeChunk->size - size < sizeof ( pageFileLocation ) ) { //Memory to manage free space exceeds free space (actually more than)
        //Thus, use the free chunk for your data.
        swapFree -= freeChunk->size; //Account for not mallocable overhead
        freeChunk->size = size;
        return freeChunk;
    } else {
        swapFree -= size; //Account for not mallocable overhead
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
    bool endIsReached = false;
    do { //We possibly need multiple frees.
        pageFileLocation *next = pagePtr->glob_off_next;
        endIsReached = ( pagePtr->status == PAGE_END );
        global_offset goff = determineGlobalOffset ( *pagePtr );
        auto it = all_space.find ( goff );
        swapFree += pagePtr->size;
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
        goff = determineGlobalOffset ( *pagePtr );
        it = all_space.find ( goff );
        ++it;
        //Check if we have unusable space after us to reuse:
        global_offset nextoff = ( it == all_space.end() ? swapSize : determineGlobalOffset ( * ( it->second ) ) );
        global_bytesize size = nextoff - goff;
        if ( pagePtr->size != size ) {
            swapFree += size - pagePtr->size;
            pagePtr->size = size;
        };

        //Check if we have free space after us to merge with;

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
        managedMemory::signalSwappingCond();
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
        ///\todo implement swapOUt if we already hold a memory copy.
        throw unfinishedCodeException ( "Swap out for read only memory chunk" );
    } else {
        pageFileLocation *newAlloced = pfmalloc ( chunk->size );
        if ( newAlloced ) {
            chunk->swapBuf = newAlloced;
            copyMem ( *newAlloced, chunk->locPtr );
            free ( chunk->locPtr );///\todo move allocation and free to managedMemory...
            chunk->locPtr = NULL; // not strictly required.
            chunk->status = MEM_SWAPPED;
            swapUsed += chunk->size;
            managedMemory::signalSwappingCond();
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
pageFileLocation managedFileSwap::determinePFLoc ( global_offset g_offset, global_bytesize length )
{
    pageFileLocation pfLoc(g_offset / pageFileSize,g_offset - pfLoc.file * pageFileSize,length,PAGE_UNKNOWN_STATE);
    return pfLoc;
}



void managedFileSwap::scheduleCopy( pageFileLocation &ref, void* ramBuf, bool reverse)
{
  if(reverse){
    //Check what is to check for a copy from ref to ramBuf:
    if(ref.status&MEM_ALLOCATED)
      throw memoryException("Cannot copy in buffer that is already allocated");
    
  }else{
    if(ref.status&MEM_SWAPPED)
      throw memoryException("Cannot copy out buffer that is already swapped");
    
  }
  ref.aio_ptr = new struct aiocb;
  ref.aio_ptr->aio_buf = ramBuf;
  ref.aio_ptr->aio_fildes = fileno(swapFiles[ref.file]);
  ref.aio_ptr->aio_nbytes = ref.size;
  ref.aio_ptr->aio_offset = ref.offset;
  ref.aio_ptr->aio_reqprio = 0;///@todo: make this configurable
  ref.aio_ptr->aio_sigevent = evhandler;
  ref.aio_ptr->aio_sigevent.sigev_value.sival_ptr = &ref;
}


void managedFileSwap::copyMem (  pageFileLocation &ref, void *ramBuf )
{
    pageFileLocation *cur = &ref;
    char *cramBuf = ( char * ) ramBuf;
    global_bytesize offset = 0;
    while ( true ) { //Sift through all pageChunks that have to be read
        global_offset g_off = determineGlobalOffset ( *cur );

        global_bytesize pageChunkSize = cur->size;
	offset+=cur->size;
	scheduleCopy(*cur,(void*)(cramBuf + offset));
        if ( cur->status == PAGE_END ) {//I have completely written this pageChunk.
            break;
        }
        cur = cur->glob_off_next;
    };
}

void managedFileSwap::copyMem ( void *ramBuf,  pageFileLocation &ref )
{
    pageFileLocation *cur = &ref;
    char *cramBuf = ( char * ) ramBuf;
    global_bytesize offset = 0;
    while ( true ) { //Sift through all pageChunks that have to be read
        global_offset g_off = determineGlobalOffset ( *cur );

        global_bytesize pageChunkSize = cur->size;
	offset+=cur->size;
	scheduleCopy(cramBuf + offset,*cur);
        if ( cur->status == PAGE_END ) {//I have completely written this pageChunk.
            break;
        }
        cur = cur->glob_off_next;
    };
}
managedFileSwap *managedFileSwap::instance = NULL;

}

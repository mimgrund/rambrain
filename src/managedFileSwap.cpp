#include "managedFileSwap.h"
#include "common.h"
#include <unistd.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include "exceptions.h"
#include "managedMemory.h"
#include "membrain_atomics.h"
#include <libaio.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mm_malloc.h>

namespace membrain
{

//#define DBG_AIO
managedFileSwap::managedFileSwap ( global_bytesize size, const char *filemask, global_bytesize oneFile ) : managedSwap ( size ), pageSize ( sysconf ( _SC_PAGE_SIZE ) )
{


    if ( oneFile == 0 ) { // Layout this on your own:

        global_bytesize myg = size / 16;
        oneFile = min ( gig, myg );
        oneFile = max ( mib, oneFile );
    }

    oneFile += ( memoryAlignment - oneFile & memoryAlignment );
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
        pageFileLocation *pfloc = new pageFileLocation ( n, 0, pageFileSize );
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

    aio_eventarr = ( struct io_event * ) malloc ( sizeof ( struct io_event ) * aio_max_transactions );
    memset ( aio_eventarr, 0, sizeof ( struct io_event ) *aio_max_transactions );
    int ioSetupErr = io_setup ( aio_max_transactions, &aio_context );
    if ( 0 != ioSetupErr ) {
        throw ( memoryException ( "Could not initialize aio!" ) );
    }
    memset ( &aio_template, 0, sizeof ( aio_template ) );
    aio_template.aio_reqprio = 0;
}

managedFileSwap::~managedFileSwap()
{
    free ( aio_eventarr );
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
            close ( swapFiles[n].fileno );
        }
        free ( swapFiles );
    }


    for ( unsigned int n = 0; n < pageFileNumber; ++n ) {
        char fname[1024];
        snprintf ( fname, 1024, filemask, n );
        unlink ( fname );
    }

}


bool managedFileSwap::openSwapFiles()
{
    if ( swapFiles ) {
        throw memoryException ( "Swap files already opened. Close first" );
        return false;
    }
    ///\todo Limit number of open swap file descriptors to something reasonable (<100?)
    swapFiles = ( struct swapFileDesc * ) malloc ( sizeof ( struct swapFileDesc ) * pageFileNumber );
    for ( unsigned int n = 0; n < pageFileNumber; ++n ) {
        char fname[1024];
        snprintf ( fname, 1024, filemask, n );
        swapFiles[n].fileno = open ( fname, O_RDWR | O_TRUNC | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR );
        if ( swapFiles[n].fileno == -1 ) {

            printf ( "Toerooooe   %d    eeh\n", errno );
            throw memoryException ( "Could not open swap file." );
            return false;
        }
        ///@todo; find out how we can dynamically query block size for dma: size_t locali = ioctl(swapFiles[n].fileno,BLKSZGET);
        swapFiles[n].currentSize = 0;
    }
    return true;
}

pageFileLocation *managedFileSwap::pfmalloc ( global_bytesize size, managedMemoryChunk *chunk )
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
        res->glob_off_next.chunk = chunk;
    } else { //We need to write out the data in parts.


        //check for enough space:
        global_bytesize total_space = 0;
        it = free_space.begin();
        do {
            total_space -= total_space % memoryAlignment; // We have to padd splitted chunks.
            total_space += it->second->size;
        } while ( total_space < size && ++it != free_space.end() );

        if ( total_space >= size ) { //We can concat enough free chunks to satisfy memory requirements
            it = free_space.begin();
            while ( true ) {
                global_bytesize avail_space = it->second->size;
                global_bytesize alloc_here = min ( avail_space, size );
                if ( size > alloc_here ) {
                    alloc_here -= alloc_here % memoryAlignment;
                }
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
                    former->glob_off_next.glob_off_next = neu;
                }
                if ( size == 0 ) {
                    neu->glob_off_next.chunk = chunk;
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
    global_bytesize padded_size = ( size / memoryAlignment + ( size % memoryAlignment == 0 ? 0 : 1 ) ) * memoryAlignment;
    if ( padded_size != size ) {
        if ( padded_size > freeChunk->size ) {
            return NULL;
        }
        printf ( "oink!\n" );
        membrain_atomic_fetch_sub ( &swapFree, padded_size - size );
    }

    if ( freeChunk->size - padded_size < sizeof ( pageFileLocation ) ) { //Memory to manage free space exceeds free space (actually more than)
        //Thus, use the free chunk for your data.
        membrain_atomic_fetch_sub ( &swapFree, freeChunk->size - padded_size ); //Account for not mallocable overhead, rest is done by claimUse
        freeChunk->size = size;
        return freeChunk;
    } else {
        pageFileLocation *neu = new pageFileLocation ( *freeChunk );
        freeChunk->offset += padded_size;
        freeChunk->size -= padded_size;
        freeChunk->glob_off_next.glob_off_next = NULL;
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
        pageFileLocation *next = pagePtr->glob_off_next.glob_off_next;
        endIsReached = ( pagePtr->status == PAGE_END );
        global_offset goff = determineGlobalOffset ( *pagePtr );
        auto it = all_space.find ( goff );
        //membrain_atomic_fetch_add ( &swapFree, pagePtr->size );

        //Delete possible pending aio_requests
        //Check whether we're about to be deleted

        if ( pagePtr->aio_ptr ) { //Pending aio
            while ( pagePtr->aio_lock != 0 ) {};
        }




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
            membrain_atomic_fetch_add ( &swapFree, size - pagePtr->size );
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
    claimUsageof ( chunk->size, false, false );
}

global_bytesize managedFileSwap::swapIn ( managedMemoryChunk *chunk )
{
    void *buf = _mm_malloc ( chunk->size, memoryAlignment );
    if ( !chunk->swapBuf ) {
        return 0;
    }
    if ( buf ) {
        if ( chunk->status & MEM_ALLOCATED || chunk->status == MEM_SWAPIN ) {
            return chunk->size;    //chunk is available or will become available
        }
        chunk->locPtr = buf;
        claimUsageof ( chunk->size, true, true );
        chunk->status = MEM_SWAPIN;
        copyMem ( chunk->locPtr, * ( ( pageFileLocation * ) chunk->swapBuf ) );
        return chunk->size;
    } else {
        return 0;
    }
}

global_bytesize managedFileSwap::swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    global_bytesize n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += swapIn ( chunklist[n] ) ;
    }
    return n_swapped;
}

global_bytesize managedFileSwap::swapOut ( managedMemoryChunk *chunk )
{
    if ( chunk->size > swapFree ) {
        return 0;
    }
    if ( chunk->status == MEM_SWAPPED || chunk->status == MEM_SWAPOUT ) {
        return chunk->size;    //chunk is or will be swapped
    }
    if ( chunk->swapBuf ) { //We already have a position to store to! (happens when read-only was triggered)
        ///\todo implement swapOUt if we already hold a memory copy.
        throw unfinishedCodeException ( "Swap out for read only memory chunk" );
    } else {
        pageFileLocation *newAlloced = pfmalloc ( chunk->size, chunk );
        if ( newAlloced ) {
            chunk->swapBuf = newAlloced;
            claimUsageof ( chunk->size, false, true );
            managedMemory::defaultManager->claimTobefreed ( chunk->size, true );
            chunk->status = MEM_SWAPOUT;
            copyMem ( *newAlloced, chunk->locPtr );
            return chunk->size;
        } else {
            return 0;
        }
    }
    return 0;

}

global_bytesize managedFileSwap::swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks )
{
    global_bytesize n_swapped = 0;
    for ( unsigned int n = 0; n < nchunks; ++n ) {
        n_swapped += swapOut ( chunklist[n] );
    }
    return n_swapped;
}


global_offset managedFileSwap::determineGlobalOffset ( const pageFileLocation &ref )
{
    return ref.file * pageFileSize + ref.offset;
}
pageFileLocation managedFileSwap::determinePFLoc ( global_offset g_offset, global_bytesize length )
{
    pageFileLocation pfLoc ( g_offset / pageFileSize, g_offset - pfLoc.file * pageFileSize, length, PAGE_UNKNOWN_STATE );
    return pfLoc;
}



void managedFileSwap::scheduleCopy ( pageFileLocation &ref, void *ramBuf, int *tracker, bool reverse )
{

    //We possibly need to resize swap file:
    global_bytesize neededSize = ref.size + ref.offset;
    if ( neededSize > swapFiles[ref.file].currentSize ) { // We need to resize swapFileDesc
        global_bytesize resizeStep = pageFileSize * swapFileResizeFrac;
        neededSize = neededSize % ( resizeStep ) == 0 ? neededSize : resizeStep * ( neededSize / resizeStep + 1 );

        ftruncate ( swapFiles[ref.file].fileno, neededSize );
        swapFiles[ref.file].currentSize = neededSize;
    }
    membrain_atomic_fetch_add ( tracker, 1 );
    membrain_atomic_fetch_add ( &totalSwapActionsQueued, 1 );
    ref.aio_ptr = new struct aiotracker;

    ref.aio_lock = 1;

    struct iocb *aio = & ( ref.aio_ptr->aio );
    *aio = aio_template;
    aio->aio_fildes = swapFiles[ref.file].fileno;
    aio->u.c.nbytes = ref.size;
    aio->u.c.offset = ref.offset;
    aio->u.c.buf = ramBuf;
    ref.aio_ptr->tracker = tracker;
#ifdef DBG_AIO
    reverse ? printf ( "scheduling read\n" ) : printf ( "scheduling write\n" );
#endif

    aio->aio_lio_opcode = reverse ? IO_CMD_PREAD : IO_CMD_PWRITE; ///@todo: a potential vector read/write may be superior
    int res = io_submit ( aio_context, 1, & ( aio ) );
    pendingAios[aio] = &ref;

    if ( res != 1 ) {
        throw memoryException ( "Could not enqueue request" );
    }

}

void managedFileSwap::completeTransactionOn ( pageFileLocation *ref, bool lock )
{
#ifdef DBG_AIO
    printf ( "got a call\n" );
#endif
    while ( ref->status != PAGE_END ) {
        ref = ref->glob_off_next.glob_off_next;
    }
    managedMemoryChunk *chunk = ref->glob_off_next.chunk;
    switch ( chunk->status ) {
    case MEM_SWAPIN:
#ifdef DBG_AIO
        printf ( "Accounting for a swapin\n" );
#endif
        if ( lock ) {
            pthread_mutex_lock ( &managedMemory::defaultManager->stateChangeMutex );
        }
        pffree ( ( pageFileLocation * ) chunk->swapBuf );
        chunk->swapBuf = NULL; // Not strictly required
        //if we have a user for this object, protect it from being swapped out again
        chunk->status = chunk->useCnt == 0 ? MEM_ALLOCATED : MEM_ALLOCATED_INUSE_READ;
        claimUsageof ( chunk->size, false, false );
        managedMemory::signalSwappingCond();
#ifdef SWAPSTATS
        membrain_atomic_fetch_add ( & ( managedMemory::defaultManager->swap_in_bytes ), chunk->size );
#endif
        if ( lock ) {
            pthread_mutex_unlock ( &managedMemory::defaultManager->stateChangeMutex );
        }
        break;
    case MEM_SWAPOUT:
#ifdef DBG_AIO
        printf ( "Accounting for a swapout\n" );
#endif
        if ( lock ) {
            pthread_mutex_lock ( &managedMemory::defaultManager->stateChangeMutex );
        }
        _mm_free ( chunk->locPtr );///\todo move allocation and free to managedMemory...
        chunk->locPtr = NULL; // not strictly required.
        chunk->status = MEM_SWAPPED;
        claimUsageof ( chunk->size, true, false );
#ifdef SWAPSTATS
        membrain_atomic_fetch_add ( & ( managedMemory::defaultManager->swap_out_bytes ), chunk->size );
#endif
        managedMemory::defaultManager->claimTobefreed ( chunk->size, false );
        managedMemory::signalSwappingCond();
        if ( lock ) {
            pthread_mutex_unlock ( &managedMemory::defaultManager->stateChangeMutex );
        }
        break;
    default:
        throw memoryException ( "AIO Synchronization broken!" );
        break;
    }

}


bool managedFileSwap::checkForAIO()
{

    bool ret = false;
    //This may be called by different threads. We only want one waiting for aio-arrivals, the others may continue.
    if ( 0 != pthread_mutex_trylock ( &aioWaiterLock ) ) {
        return false;
    }

    //We're the only thread here. Check if there's still pending requests to wait for:
    if ( totalSwapActionsQueued == 0 ) { //We do not need to wait as nothing is coming.
        return true;    //Do not wait in caller for something to arrive.
    }

#ifdef DBG_AIO
    printf ( "checkForAIO called, we'll wait for an event, got %d in queue\n", totalSwapActionsQueued );
#endif
    //As there's at least one pending transaction, we may wait blocking indefinitely:
    pthread_mutex_unlock ( & ( managedMemory::defaultManager->stateChangeMutex ) );
    int no_arrived = io_getevents ( aio_context, 1, aio_max_transactions, aio_eventarr, NULL );
    pthread_mutex_lock ( & ( managedMemory::defaultManager->stateChangeMutex ) );
    if ( no_arrived < 0 ) {
        pthread_mutex_unlock ( &aioWaiterLock );
        printf ( "We got an error back: %d\n", -no_arrived );
        throw memoryException ( "AIO Error" );

    }
#ifdef DBG_AIO
    printf ( "we got %d events\n", no_arrived );
#endif
    for ( int n = 0; n < no_arrived; ++n ) {
        //Try to find mapping:
#ifdef DBG_AIO
        printf ( "Processing event %d \n", n );
#endif
        auto found = pendingAios.find ( aio_eventarr[n].obj );
        if ( found != pendingAios.end() ) {
            pageFileLocation *ref = found->second;
            pendingAios.erase ( found );
            //Deal with element:
            asyncIoArrived ( ref, aio_eventarr + n );
        }

    }


    pthread_mutex_unlock ( &aioWaiterLock );
    return true;

};

void managedFileSwap::asyncIoArrived ( membrain::pageFileLocation *ref, io_event *event )
{

#ifdef DBG_AIO
    printf ( "aio: async io arrived, chunk of size %d\n", ref->size );
#endif

    //Check whether we're about to be deleted, if so, don't touch the element
    int *tracker = ref->aio_ptr->tracker;
    //Check if aio was completed:


    int err = event->res2; //Seems to be that a value of zero here indicates success.
    if ( err == 0 && event->res == ref->size ) { //This part arrived successfully
        delete ref->aio_ptr;
        ref->aio_ptr = NULL;
        ref->aio_lock = 0;

        int lastval = membrain_atomic_fetch_sub ( tracker, 1 );
        if ( lastval == 1 ) {
            completeTransactionOn ( ref , false );
            delete tracker;
        }

        membrain_atomic_fetch_sub ( &totalSwapActionsQueued, 1 ); //Do this at the very last line, as completeTransactionOn() has to be done beforehands.

    } else {

        errmsgf ( "We have trouble in chunk %d, %d ; aio_size %d, size %d", ref->glob_off_next.chunk->id, err, event->res, ref->size );
        errmsgf ( "file-align %d, buf-align %d", ref->offset % memoryAlignment );

        ///@todo: find out if this may happen regularly or just in case of error.
    }


}



void managedFileSwap::copyMem (  pageFileLocation &ref, void *ramBuf , bool reverse )
{
    pageFileLocation *cur = &ref;
    char *cramBuf = ( char * ) ramBuf;
    global_bytesize offset = 0;
    int *tracker = new int;
    *tracker = 1;
    membrain_atomic_fetch_add ( &totalSwapActionsQueued, 1 );
    while ( true ) { //Sift through all pageChunks that have to be read
        scheduleCopy ( *cur, ( void * ) ( cramBuf + offset ), tracker, reverse );
        if ( cur->status == PAGE_END ) {//I have completely written this pageChunk.
            break;
        }
        offset += cur->size;
        cur = cur->glob_off_next.glob_off_next;
    };
    unsigned int trval = membrain_atomic_fetch_sub ( tracker, 1 );
    membrain_atomic_fetch_sub ( &totalSwapActionsQueued, 1 );
    if ( trval == 1 ) {
        completeTransactionOn ( cur , false ); //We already call having aquired the lock and know that nothing fatal happens
        delete tracker;
    }
}

managedFileSwap *managedFileSwap::instance = NULL;

void managedFileSwap::sigStat ( int signum )
{
    global_bytesize total_space = instance->swapSize;
    global_bytesize free_space = 0;
    global_bytesize free_space2 = 0;
    global_bytesize fractured = 0;
    global_bytesize partend = 0;
    auto it = instance->free_space.begin();
    do {
        free_space += it->second->size;
    } while ( ++it != instance->free_space.end() );

    it = instance->all_space.begin();
    do {
        switch ( it->second->status ) {
        case PAGE_END:
            partend += it->second->size;
            break;
        case PAGE_FREE:
            free_space2 += it->second->size;
            break;
        case PAGE_PART:
            fractured += it->second->size;
            break;
        default:
            break;
        }
    } while ( ++it != instance->all_space.end() );
    const char *text;
    if ( free_space == instance->swapFree ) {
        text = "sane";
    } else {
        text = "insane";
    }

    printf ( "%ld\t%ld\t%ld\t%e\t%e\t%s\n", free_space, partend, fractured, ( ( double ) free_space ) / ( partend + fractured + free_space ), ( ( ( double ) ( total_space ) - ( partend + fractured + free_space ) ) / ( total_space ) ), ( free_space == instance->swapFree ? "sane" : "insane" ) );


}



}


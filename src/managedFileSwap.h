#ifndef MANAGEDFILESWAP_H
#define MANAGEDFILESWAP_H

#include "managedSwap.h"
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <queue>
#include <libaio.h>
#include <signal.h>
#include <unordered_map>
#include <sys/types.h>
#include <unistd.h>

//Test classes
class managedFileSwap_Unit_SwapAllocation_Test;
class managedFileSwap_Integration_RandomAccess_Test;
class managedFileSwap_Integration_RandomAccessVariousSize_Test;

namespace rambrain
{
///@brief the status for pageFileLocations
enum pageChunkStatus {PAGE_FREE = 1 /** PageChunk is free space not occupied by a swapped chunk **/,
                      PAGE_PART = 2 /** PageChunk is part of an object **/,
                      PAGE_END = 4 /** PageChunk is last part of an object ( or whole object) **/,
                      PAGE_WASREAD = 8 /** PageChunk has been read by user**/,
                      PAGE_UNKNOWN_STATE = 16 /** PageChunk has unknown state ( temporary ) **/
                     };

typedef uint64_t global_offset;

class pageFileLocation;

///@brief structure to handle swap files
struct swapFileDesc {
    int fileno;
    global_bytesize currentSize;
};

///@brief datastructure for handling asynchronous events
struct aiotracker {
    struct iocb aio;
    int *tracker;
};

///@brief saves some storage in pageFileLocation
union glob_off_union {
    pageFileLocation *glob_off_next;
    managedMemoryChunk *chunk;
    glob_off_union() {
        chunk = NULL;
    };
};


/** @brief tracks page file allocations
 * while objects are preferably written continuous to page file, we may encounter the situation that the
 * pagefiles are nearly full. In this case we take fragments of free space and use these to break up the
 * consecutive memory of a chunk into parts, tracked by pageChunks.
 * */
class pageFileLocation
{
public:
    pageFileLocation ( unsigned int file, global_bytesize offset, global_bytesize size, pageChunkStatus status = PAGE_FREE ) :
        file ( file ), offset ( offset ), size ( size ), status ( status ), aio_ptr ( NULL ) {}

    unsigned int file;
    global_bytesize offset;
    global_bytesize size;
    union glob_off_union glob_off_next;//This points if used to the next part, if free to the next free chunk, if PAGE_END points to memchunk.
    pageChunkStatus status;
    struct aiotracker *aio_ptr = NULL;
    char aio_lock = 0;
};



class managedFileSwap;


class managedFileSwap : public managedSwap
{
public:

    managedFileSwap ( global_bytesize size, const char *filemask, global_bytesize oneFile = 0, bool enableDMA = false );
    virtual ~managedFileSwap();

    virtual void swapDelete ( managedMemoryChunk *chunk );
    virtual global_bytesize swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapIn ( managedMemoryChunk *chunk );
    virtual global_bytesize swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapOut ( managedMemoryChunk *chunk );
    void setDMA ( bool arg1 );

    const unsigned int pageSize;


private:
    pageFileLocation determinePFLoc ( global_offset g_offset, global_bytesize length ) const;
    inline global_offset determineGlobalOffset ( const pageFileLocation &ref ) const;
    bool openSwapFiles();
    void closeSwapFiles();

    const char *filemask;

    unsigned int pageFileNumber;
    global_bytesize pageFileSize;


    float swapFileResizeFrac = .1; ///@todo: Possibly make this configurable?

    struct swapFileDesc *swapFiles = NULL;

    //Memory copy:
    void scheduleCopy ( rambrain::pageFileLocation &ref, void *ramBuf, int *tracker, bool reverse = false ) ;
    void copyMem ( rambrain::pageFileLocation &ref, void *ramBuf, bool reverse = false  )  ;

    inline void copyMem ( void *ramBuf, rambrain::pageFileLocation &ref ) {
        copyMem ( ref, ramBuf, true );
    }

    inline void scheduleCopy ( void *ramBuf, pageFileLocation &ref, int *parttracker ) {
        scheduleCopy ( ref, ramBuf, parttracker, true );
    }

    inline size_t getMemoryAlignment() const {
        return memoryAlignment;
    }

    bool filesOpen = false;


    //page file malloc:
    pageFileLocation *pfmalloc ( rambrain::global_bytesize size, rambrain::managedMemoryChunk *chunk );
    void pffree ( pageFileLocation *pagePtr );
    pageFileLocation *allocInFree ( pageFileLocation *freeChunk, global_bytesize size );

    std::map<global_offset, pageFileLocation *> free_space;
    std::map<global_offset, pageFileLocation *> all_space;


    bool enableDMA = false;
protected:
    bool deleteFilesOnExit = true;

    //sigEvent Handler:
    void asyncIoArrived ( rambrain::pageFileLocation *ref, struct io_event *aio );
    void completeTransactionOn ( rambrain::pageFileLocation *ref, bool lock = true );

    virtual bool checkForAIO();
    static void *checkForAIO ( void *ptr );

    struct iocb aio_template;
    io_context_t aio_context = 0;
    unsigned int aio_max_transactions = 1024;
    struct io_event *aio_eventarr;
    pthread_mutex_t aioWaiterLock = PTHREAD_MUTEX_INITIALIZER;

    std::unordered_map<struct iocb *, pageFileLocation *> pendingAios;

    //Test classes
    friend class ::managedFileSwap_Unit_SwapAllocation_Test;
    friend class ::managedFileSwap_Integration_RandomAccess_Test;
    friend class ::managedFileSwap_Integration_RandomAccessVariousSize_Test;

    static managedFileSwap *instance;
    static void sigStat ( int signum );

    //Thread pool for asynchronous io:
    pthread_mutex_t io_submit_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t io_submit_cond = PTHREAD_COND_INITIALIZER;
    unsigned int io_submit_num_threads = 1;
    pthread_t *io_submit_threads;
    pthread_t io_waiter_thread;

    std::queue<struct iocb *> io_submit_requests;

    void my_io_submit ( struct iocb *aio );
    static void *io_submit_worker ( void *ptr );

    bool cleanupCachedElements ( rambrain::global_bytesize minimum_size = 0 );
    virtual void invalidateCacheFor ( managedMemoryChunk &chunk );
};

}


#endif

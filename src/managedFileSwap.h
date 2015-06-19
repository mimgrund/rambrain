#ifndef MANAGEDFILESWAP_H
#define MANAGEDFILESWAP_H

#include "managedSwap.h"
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <libaio.h>
#include <signal.h>
#include <unordered_map>

//Test classes
class managedFileSwap_Unit_SwapAllocation_Test;
class managedFileSwap_Integration_RandomAccess_Test;
class managedFileSwap_Integration_RandomAccessVariousSize_Test;

namespace membrain
{

enum pageChunkStatus {PAGE_FREE = 1,
                      PAGE_PART = 2,
                      PAGE_END = 4,
                      PAGE_WASREAD = 8,
                      PAGE_UNKNOWN_STATE = 16
                     };

typedef uint64_t global_offset;

class pageFileLocation;

struct swapFileDesc {
    //FILE *descriptor;
    int fileno;
    global_bytesize currentSize;
};

struct aiotracker {
    struct iocb aio;
    int *tracker;
};

union glob_off_union {

    pageFileLocation *glob_off_next;
    managedMemoryChunk *chunk;
    glob_off_union() {
        chunk = NULL;
    };
};

//This will be stored in managedMemoryChunk::swapBuf :
class pageFileLocation
{
public:
    pageFileLocation ( unsigned int file, global_bytesize offset, global_bytesize size, pageChunkStatus status = PAGE_FREE ) : file ( file ), offset ( offset ), size ( size ), status ( status ), aio_ptr ( NULL ) {};
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

    managedFileSwap ( global_bytesize size, const char *filemask, global_bytesize oneFile = 0 );
    ~managedFileSwap();

    virtual void swapDelete ( managedMemoryChunk *chunk );
    virtual global_bytesize swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapIn ( managedMemoryChunk *chunk );
    virtual global_bytesize swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapOut ( managedMemoryChunk *chunk );

    const unsigned int pageSize;

private:
    pageFileLocation determinePFLoc ( global_offset g_offset, global_bytesize length );
    inline global_offset determineGlobalOffset ( const pageFileLocation &ref );
    bool openSwapFiles();
    void closeSwapFiles();

    const char *filemask;

    unsigned int pageFileNumber;
    global_bytesize pageFileSize;

    unsigned int windowSize;
    unsigned int windowNumber;
    unsigned int lastCreatedWindow = 0;



    float swapFileResizeFrac = .1; ///@todo: Possibly make this configurable?

    struct swapFileDesc *swapFiles = NULL;

    //Memory copy:
    void scheduleCopy ( membrain::pageFileLocation &ref, void *ramBuf, int *parttracker, bool reverse = false );
    void copyMem ( membrain::pageFileLocation &ref, void *ramBuf, bool reverse = false  )  ;

    inline void copyMem ( void *ramBuf, membrain::pageFileLocation &ref ) {
        copyMem ( ref, ramBuf, true );
    };
    inline void scheduleCopy ( void *ramBuf, pageFileLocation &ref, int *parttracker ) {
        scheduleCopy ( ref, ramBuf, parttracker, true );
    };


    bool filesOpen = false;


    //page file malloc:
    pageFileLocation *pfmalloc ( membrain::global_bytesize size, membrain::managedMemoryChunk *chunk );
    void pffree ( pageFileLocation *pagePtr );
    pageFileLocation *allocInFree ( pageFileLocation *freeChunk, global_bytesize size );

    std::map<global_offset, pageFileLocation *> free_space;
    std::map<global_offset, pageFileLocation *> all_space;


protected:
    bool deleteFilesOnExit = true;

    //sigEvent Handler:
    void asyncIoArrived ( membrain::pageFileLocation *ref, struct io_event *aio );
    void completeTransactionOn ( membrain::pageFileLocation *ref, bool lock = true );
    virtual bool checkForAIO();
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
};

}


#endif

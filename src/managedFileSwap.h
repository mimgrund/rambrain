#ifndef MANAGEDFILESWAP_H
#define MANAGEDFILESWAP_H
#include "managedSwap.h"
#include <stdio.h>

enum pageChunkStatus {PAGE_FREE = 1,
                      PAGE_PART = 2,
                      PAGE_END = 4,
                      PAGE_WASREAD = 8
                     };


//This will be stored in managedMemoryChunk::swapBuf :
struct pageFileLocationStruct {
    unsigned int file;
    unsigned int offset;
    unsigned int size;
    struct pageFileLocationStruct *next_prt;
    struct pageFileLocationStruct *next_glob;
    struct pageFileLocationStruct *prev_glob;
    pageChunkStatus status;
};

typedef struct pageFileLocationStruct pageFileLocation;

class managedFileSwap;

class pageFileWindow
{
public:
    pageFileWindow ( const pageFileLocation &location, managedFileSwap &swap );
    ~pageFileWindow();


    void triggerSync ( bool async = true );
    float percentageClean();
    void *getMem ( const pageFileLocation &loc );
private:
    void *buf;
    unsigned int offset;
    unsigned int length;
    unsigned int file;
};




class managedFileSwap : public managedSwap
{
public:

    managedFileSwap ( unsigned int size, const char *filemask, unsigned int oneFile = 1024 );
    ~managedFileSwap();

    virtual void swapDelete ( managedMemoryChunk *chunk );
    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapIn ( managedMemoryChunk *chunk );
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapOut ( managedMemoryChunk *chunk );

    static const unsigned int pageSize;

private:
    pageFileLocation determinePFLoc ( unsigned int g_offset, unsigned int length );
    bool openSwapFiles();
    void closeSwapFiles();

    const char *filemask;

    unsigned int pageFileNumber;
    unsigned int pageFileSize;

    unsigned int windowSize;
    unsigned int windowNumber;


    FILE **swapFiles = NULL;
    pageFileWindow **windows = NULL;
    void *getMem ( const pageFileLocation & );

    bool filesOpen = false;


    //page file malloc:
    pageFileLocation *pfmalloc ( unsigned int size );
    void pffree ( pageFileLocation *pagePtr );
    pageFileLocation *allocInFree ( pageFileLocation *freeChunk, pageFileLocation *prevChunk, unsigned int size );

    pageFileLocation *free_entry = NULL;

    friend pageFileWindow;

};


#endif
#ifndef MANAGEDFILESWAP_H
#define MANAGEDFILESWAP_H
#include "managedSwap.h"
#include <stdio.h>

enum pageChunkStatus {PAGE_PART_START,
                      PAGE_PART_END,
                      PAGE_FREE
                     };


struct pageFileLocationStruct {
    unsigned int file;
    unsigned int offset;
    unsigned int size;
    struct pageFileLocationStruct *next;
    struct pageFileLocationStruct **freetracker;
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
    pageFileLocation *free_entry = NULL;

    friend pageFileWindow;

};


#endif
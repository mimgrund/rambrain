#ifndef MANAGEDFILESWAP_H
#define MANAGEDFILESWAP_H
#include "managedSwap.h"


enum pageChunkStatus {PAGE_PART_START,
                      PAGE_PART_END,
                      PAGE_FREE
                     };

};

struct pageFileLocation {
    unsigned int file;
    unsigned int pm_page;
    unsigned int offset;
    unsigned int size;
    pageFileLocation *next;
    pageFileLocation **freetracker;
};



class pageFileWindow
{
public:
    pageFileWindow ( FILE *, unsigned int pm_offset, unsigned int pm_length );
    ~pageFileWindow();


    void triggerSync ( bool async = true );
    float percentageClean();
    void *getMem ( const struct pageFileLocation & );


};




class managedFileSwap : public managedSwap
{
public:

    managedFileSwap ( unsigned int size, const char *filemask, unsigned int oneFileMB = 1024 );
    ~managedFileSwap();

    virtual void swapDelete ( managedMemoryChunk *chunk );
    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapIn ( managedMemoryChunk *chunk );
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapOut ( managedMemoryChunk *chunk );

private:
    struct pageFileLocation determineOffsets ( unsigned int g_offset, unsigned int length );
    bool openSwapFiles();
    void closeSwapFiles();

    const char *filemask;
    const unsigned int pageSize;
    unsigned int pageFileNumber;
    unsigned int fileSize;

    unsigned int windowSize;
    unsigned int windowNumber;


    FILE **swapFiles = NULL;
    pageFileWindow **windows = NULL;
    void *getMem ( const struct pageFileLocation & );

    bool filesOpen = false;


    //page file malloc:
    struct pageFileLocation *pfmalloc ( unsigned int size );
    void pffree ( pageFileLocation *pagePtr );
    struct pageFileLocation *free_entry = NULL;


};


#endif
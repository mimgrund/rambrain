#ifndef MANAGEDFILESWAP_H
#define MANAGEDFILESWAP_H
#include "managedSwap.h"
#include <stdio.h>
#include <map>
enum pageChunkStatus {PAGE_FREE = 1,
                      PAGE_PART = 2,
                      PAGE_END = 4,
                      PAGE_WASREAD = 8,
                      PAGE_UNKNOWN_STATE = 16
                     };

typedef unsigned int global_offset;

//This will be stored in managedMemoryChunk::swapBuf :
struct pageFileLocationStruct {
    unsigned int file;
    unsigned int offset;
    unsigned int size;
    struct pageFileLocationStruct *glob_off_next;//This points if used to the next part, if free to the next free chunk.
    pageChunkStatus status;
};
typedef struct pageFileLocationStruct pageFileLocation;



class managedFileSwap;

class pageFileWindow
{
public:
    pageFileWindow ( const pageFileLocation &location, managedFileSwap &swap );
    ~pageFileWindow();

    bool isInWindow ( const pageFileLocation &location, unsigned int &offset_start, unsigned int &offset_end );
    bool isInWindow ( const pageFileLocation &location, unsigned int &length );
    bool isInWindow ( const pageFileLocation );

    void triggerSync ( bool async = true );
    float percentageClean();
    void *getMem ( const pageFileLocation &loc, unsigned int &size );
private:
    void *buf;
    unsigned int offset;
    unsigned int length;
    unsigned int file;
    unsigned int window;
};




class managedFileSwap : public managedSwap
{
public:

    managedFileSwap ( unsigned int size, const char *filemask, unsigned int oneFile = 0 );
    ~managedFileSwap();

    virtual void swapDelete ( managedMemoryChunk *chunk );
    virtual unsigned int swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapIn ( managedMemoryChunk *chunk );
    virtual unsigned int swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual bool swapOut ( managedMemoryChunk *chunk );

    static const unsigned int pageSize;

private:
    pageFileLocation determinePFLoc ( global_offset g_offset, unsigned int length );
    inline global_offset determineGlobalOffset ( const pageFileLocation &ref );
    bool openSwapFiles();
    void closeSwapFiles();

    const char *filemask;

    unsigned int pageFileNumber;
    unsigned int pageFileSize;

    unsigned int windowSize;
    unsigned int windowNumber;
    unsigned int lastCreatedWindow = 0;

    FILE **swapFiles = NULL;

    //Window handling:
    pageFileWindow **windows = NULL;
    void *getMem ( const pageFileLocation &, unsigned int &size );
    pageFileWindow *getWindowTo ( const pageFileLocation & );

    //Memory copy:
    void copyMem ( void *ramBuf, const pageFileLocation &ref );
    void copyMem ( const pageFileLocation &ref, void *ramBuf );


    bool filesOpen = false;


    //page file malloc:
    pageFileLocation *pfmalloc ( unsigned int size );
    void pffree ( pageFileLocation *pagePtr );
    pageFileLocation *allocInFree ( pageFileLocation *freeChunk, unsigned int size );

    std::map<global_offset, pageFileLocation *> free_space;
    std::map<global_offset, pageFileLocation *> all_space;

    friend pageFileWindow;

    friend class managedFileSwap_Unit_SwapAllocation_Test;

};


#endif
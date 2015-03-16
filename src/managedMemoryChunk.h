#ifndef MANAGEDMEMORYCHUNK_H
#define MANAGEDMEMORYCHUNK_H


//memoryStatus&&MEM_ALLOCATED_INUSE_READ should return one for MEM_ALLOCATED_INUSE_WRITE, too.
enum memoryStatus {MEM_ROOT=0,
                   MEM_ALLOCATED_INUSE_READ=1,
                   MEM_ALLOCATED_INUSE_WRITE=3,
                   MEM_ALLOCATED=4,
                   MEM_SWAPPED=8
                  };

typedef unsigned int memoryID;
typedef unsigned int memoryAtime;

class managedMemoryChunk
{
public:
    managedMemoryChunk ( const memoryID &parent, const memoryID &me );

    //Local management
    memoryStatus status;
    unsigned short useCnt;
    void * locPtr;
    unsigned int size;

    //Organization
    memoryID parent;
    memoryID id;
    memoryID next;
    memoryID child;

    //Swap scheduling:
    memoryAtime atime;
    void * schedBuf;//Give the mem scheduler a place for a buffer.

    //Swap raw management:
    void * swapBuf;//Give the swapper a place for a buffer.
};

#endif
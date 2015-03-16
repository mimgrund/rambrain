#ifndef MANAGEDMEMORYCHUNK_H
#define MANAGEDMEMORYCHUNK_H

enum memoryStatus {MEM_ALLOCATED_INUSE,
                   MEM_ALLOCATED,
                   MEM_SWAPPED,
                   MEM_ROOT
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
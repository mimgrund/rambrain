#ifndef MANAGEDMEMORYCHUNK_H
#define MANAGEDMEMORYCHUNK_H

namespace membrain
{



enum memoryStatus {MEM_ROOT = 0,
                   MEM_ALLOCATED_INUSE_READ = 1,
                   MEM_ALLOCATED_INUSE_WRITE = 3,
                   MEM_ALLOCATED = 4,
                   MEM_SWAPPED = 8,
                   MEM_SWAPIN = 16,
                   MEM_SWAPOUT = 32,
                   MEM_DELETE = 64,
                   MEM_REGISTER = 128,
                   MEM_REGISTER_INUSE_READ = 129,
                   MEM_REGISTER_INUSE_WRITE = 131,
                  };

typedef unsigned int memoryID;
typedef unsigned int memoryAtime;

/** \brief manages all managed Chunks of raw memory
 *
 * This object tracks the dimesions and status of a chunk of memory we manage.
 * The status is inherently connected to the logic we can operate on the object and
 * it is basically prescribed what classes may change the status from wich state to which.
 * Even though it would be more clear to have convenience and thread-safe functions here,
 * we decided for this due to performance issues. More documentation to come. The basically
 * possible transitions are depicted in the following graph:
 * \dotfile chunkStatusScheme.dot
 * **/
class managedMemoryChunk
{
public:
    managedMemoryChunk ( const memoryID &parent, const memoryID &me );

    //Local management
    memoryStatus status;
    unsigned short useCnt;
    void *locPtr;
    unsigned int size;

    //Organization
    memoryID parent;
    memoryID id;
    memoryID next;
    memoryID child;

    //Swap scheduling:
    memoryAtime atime;
    void *schedBuf; //Give the mem scheduler a place for a buffer.

    //Swap raw management:
    void *swapBuf; //Give the swapper a place for a buffer.
};

}

#endif

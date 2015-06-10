#ifndef MANAGEDMEMORYCHUNK_H
#define MANAGEDMEMORYCHUNK_H
#include "common.h"
namespace membrain
{



enum memoryStatus {MEM_ROOT = 0,
                   MEM_ALLOCATED_INUSE_READ = 4 + 1,
                   MEM_ALLOCATED_INUSE_WRITE = 4 + 2 + 1,
                   MEM_ALLOCATED_INUSE = 1, //flaggy
                   MEM_ALLOCATED = 4, //flaggy
                   MEM_SWAPPED = 8, //flaggy
                   MEM_SWAPIN = 16 + 8,
                   MEM_SWAPOUT = 32
                  };

typedef uint64_t memoryID;
typedef uint64_t memoryAtime;

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
#ifdef PARENTAL_CONTROL
    managedMemoryChunk ( const memoryID &parent, const memoryID &me );
#else
    managedMemoryChunk ( const memoryID &me );
#endif

    //Local management
    memoryStatus status;
    unsigned short useCnt;
    void *locPtr;
    global_bytesize size;

    //Organization
    memoryID id;
#ifdef PARENTAL_CONTROL
    memoryID parent;
    memoryID next;
    memoryID child;
#endif
    bool preemptiveLoaded = false;

    //Swap scheduling:
    void *schedBuf; //Give the mem scheduler a place for a buffer.

    //Swap raw management:
    void *swapBuf; //Give the swapper a place for a buffer.
};

}

#endif

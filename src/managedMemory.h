#ifndef MANAGEDMEMORY_H
#define MANAGEDMEMORY_H

#include <stdlib.h>
#include <map>
#include <pthread.h>

#include "managedMemoryChunk.h"
#include "managedSwap.h"
#include "exceptions.h"

namespace membrain
{

template<class T>
class managedPtr;


class managedMemory
{
public:
    managedMemory ( managedSwap *swap, unsigned int size = 1073741824 );
    virtual ~managedMemory();

    //Memory Management options
    bool setMemoryLimit ( unsigned int size );
    unsigned int getMemoryLimit () const;
    unsigned int getUsedMemory() const;
    unsigned int getSwappedMemory() const;


    //Chunk Management
    bool setUse ( memoryID id );
    bool unsetUse ( memoryID id );
    bool setUse ( managedMemoryChunk &chunk, bool writeAccess );
    bool unsetUse ( managedMemoryChunk &chunk , unsigned int no_unsets = 1 );


#ifdef PARENTAL_CONTROL
    //Tree Management
    unsigned int getNumberOfChildren ( const memoryID &id );
    void printTree ( managedMemoryChunk *current = NULL, unsigned int nspaces = 0 );
    static const memoryID root;
    static memoryID parent;
    //We need the following two to correctly initialize class hierarchies:
    static bool threadSynced;
    static pthread_mutex_t parentalMutex;
    static pthread_cond_t parentalCond;
    static pthread_t creatingThread;
    static bool haveCreatingThread;
    void recursiveMfree ( memoryID id );
#else
    void linearMfree();
#endif
    static managedMemory *defaultManager;
    static const memoryID invalid;

    static unsigned int arrivedSwapins;
    static void signalSwappingCond();
protected:
    managedMemoryChunk *mmalloc ( unsigned int sizereq );
    bool mrealloc ( memoryID id, unsigned int sizereq );
    void mfree ( memoryID id );
    managedMemoryChunk &resolveMemChunk ( const memoryID &id );


    //Swapping strategy
    virtual bool swapOut ( unsigned int min_size ) = 0;
    virtual bool swapIn ( memoryID id );
    virtual bool swapIn ( managedMemoryChunk &chunk ) = 0;
    virtual bool touch ( managedMemoryChunk &chunk ) = 0;
    virtual void schedulerRegister ( managedMemoryChunk &chunk ) = 0;
    virtual void schedulerDelete ( managedMemoryChunk &chunk ) = 0;

    ///This function ensures that there is sizereq space left in RAM and locks the topoLock
    void ensureEnoughSpaceAndLockTopo ( unsigned int sizereq );

    //Swap Storage manager iface:
    managedSwap *swap = 0;

    unsigned int memory_max; //1GB
    unsigned int memory_used = 0;
    unsigned int memory_swapped = 0;

    std::map<memoryID, managedMemoryChunk *> memChunks;

    memoryAtime atime = 0;
    memoryID memID_pace = 1;


    managedMemory *previousManager;
    static bool Throw ( memoryException e );

    static pthread_mutex_t stateChangeMutex;
    //Signalled after every swapin. Synchronization is happening via stateChangeMutex
    static pthread_cond_t swappingCond;
    /*static pthread_cond_t topologicalCond;*/

    template<class T>
    friend class managedPtr;
    friend class managedSwap;

#ifdef SWAPSTATS
protected:
    global_bytesize n_swap_out = 0;
    global_bytesize n_swap_in = 0;


    global_bytesize swap_out_bytes = 0;
    global_bytesize swap_in_bytes = 0;

    global_bytesize swap_out_bytes_last = 0;
    global_bytesize swap_in_bytes_last = 0;

    global_bytesize swap_hits = 0;
    global_bytesize swap_misses = 0;

    bool waitForSwapin ( managedMemoryChunk &chunk, bool keepSwapLock = false );
    bool waitForSwapout ( managedMemoryChunk &chunk, bool keepSwapLock = false );
public:
    void printSwapstats();
    void resetSwapstats();

    static void sigswapstats ( int signum );
    static managedMemory *instance;

#endif
    static void versionInfo();
};

}

#endif

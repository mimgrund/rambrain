#ifndef MANAGEDMEMORY_H
#define MANAGEDMEMORY_H

#include <stdlib.h>
#include <map>
#include <pthread.h>

#ifdef SWAPSTATS
#include <signal.h>
#ifdef LOGSTATS
#include <stdio.h>
#include <chrono>
#endif
#endif

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
    managedMemory ( managedSwap *swap, global_bytesize size = 1073741824 );
    virtual ~managedMemory();

    //Memory Management options
    bool setMemoryLimit ( global_bytesize size );
    global_bytesize getMemoryLimit () const;
    global_bytesize getUsedMemory() const;
    global_bytesize getSwappedMemory() const;


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
    managedMemoryChunk *mmalloc ( global_bytesize sizereq );
    bool mrealloc ( memoryID id, global_bytesize sizereq );
    void mfree ( memoryID id );
    managedMemoryChunk &resolveMemChunk ( const memoryID &id );


    //Swapping strategy
    virtual bool swapOut ( global_bytesize min_size ) = 0;
    virtual bool swapIn ( memoryID id );
    virtual bool swapIn ( managedMemoryChunk &chunk ) = 0;
    virtual bool touch ( managedMemoryChunk &chunk ) = 0;
    virtual void schedulerRegister ( managedMemoryChunk &chunk ) = 0;
    virtual void schedulerDelete ( managedMemoryChunk &chunk ) = 0;

    ///This function ensures that there is sizereq space left in RAM and locks the topoLock
    void ensureEnoughSpaceAndLockTopo ( global_bytesize sizereq );

    //Swap Storage manager iface:
    managedSwap *swap = 0;

    global_bytesize memory_max; //1GB
    global_bytesize memory_used = 0;
    global_bytesize memory_swapped = 0;

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

    struct sigaction sigact;

    bool waitForSwapin ( managedMemoryChunk &chunk, bool keepSwapLock = false );
    bool waitForSwapout ( managedMemoryChunk &chunk, bool keepSwapLock = false );

#ifdef LOGSTATS
    //! @todo This will induce a serious issue in combination with MPI and a shared disk. How to handle that case?
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    //! @todo This will induce a serious issue in combination with MPI and a shared disk. How to handle that case?
    static FILE *logFile;
    static bool firstLog;
#endif

public:
    void printSwapstats();
    void resetSwapstats();

    static void sigswapstats ( int sig );
    static managedMemory *instance;

    static pthread_mutex_t swapDeletionMutex;
#endif
    static void versionInfo();
};

}

#endif

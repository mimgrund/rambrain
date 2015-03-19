#ifndef MANAGEDMEMORY_H
#define MANAGEDMEMORY_H

#include <stdlib.h>
#include <map>

#include "managedMemoryChunk.h"
#include "managedSwap.h"
#include "exceptions.h"

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
    bool unsetUse ( managedMemoryChunk &chunk , unsigned int no_unsets=1);

    //Tree Management
    unsigned int getNumberOfChildren ( const memoryID &id );
    void printTree ( managedMemoryChunk *current = NULL, unsigned int nspaces = 0 );

    static managedMemory *defaultManager;
    static const memoryID root;
    static const memoryID invalid;
    static memoryID parent;

protected:
    managedMemoryChunk *mmalloc ( unsigned int sizereq );
    bool mrealloc ( memoryID id, unsigned int sizereq );
    void mfree ( memoryID id );
    void recursiveMfree ( memoryID id );
    managedMemoryChunk &resolveMemChunk ( const memoryID &id );


    //Swapping strategy
    virtual bool swapOut ( unsigned int min_size ) = 0;
    virtual bool swapIn ( memoryID id );
    virtual bool swapIn ( managedMemoryChunk &chunk ) = 0;
    virtual bool touch ( managedMemoryChunk &chunk ) = 0;
    virtual void schedulerRegister ( managedMemoryChunk &chunk ) = 0;
    virtual void schedulerDelete ( managedMemoryChunk &chunk ) = 0;
    void ensureEnoughSpaceFor ( unsigned int sizereq );

    //Swap Storage manager iface:
    managedSwap *swap = 0;

    unsigned int memory_max; //1GB
    unsigned int memory_used = 0;
    unsigned int memory_swapped = 0;

    std::map<memoryID, managedMemoryChunk *> memChunks;

    memoryAtime atime = 0;
    memoryID memID_pace = 1;


    static managedMemory *dummyManager;
    static bool noThrow;
    static bool Throw ( memoryException e );


    template<class T>
    friend class managedPtr;

#ifdef SWAPSTATS
protected:
    unsigned int n_swap_out = 0;
    unsigned int n_swap_in = 0;
    unsigned int swap_out_bytes = 0;
    unsigned int swap_in_bytes = 0;
    unsigned int swap_hits = 0;
    unsigned int swap_misses = 0;
public:
    void printSwapstats();
    void resetSwapstats();

#endif
};

#endif






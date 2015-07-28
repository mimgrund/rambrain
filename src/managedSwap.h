#ifndef MANAGEDSWAP_H
#define MANAGEDSWAP_H

#include "managedMemoryChunk.h"
#include "common.h"
#include "managedMemory.h"
#include "rambrain_atomics.h"

namespace rambrain
{

/** @brief Class that serves as a backend to managedMemory to actual write/read managedMemoryChunks to/from hard disk or other non random access memory
 * @note when trying to write your own version, consider studying managedFileSwap as a reference implementation
 **/
class managedSwap
{
public:
    managedSwap ( global_bytesize size );
    virtual ~managedSwap();

    //Returns number of sucessfully swapped chunks
    /** @brief Trigger swap out of the chunks pointed to by chunklist
     *  @note this function must be called having stateChangeMutex acquired.
     **/
    virtual global_bytesize swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    /** @brief Trigger swap out of the chunk pointed to by chunk
     *  @note this function must be called having stateChangeMutex acquired.
     **/
    virtual global_bytesize swapOut ( managedMemoryChunk *chunk )  = 0;
    /** @brief Trigger swap in of the chunks pointed to by chunklist
     *  @note this function must be called having stateChangeMutex acquired.
     **/
    virtual global_bytesize swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks ) = 0;
    /** @brief Trigger swap in of the chunk pointed to by chunk
     *  @note this function must be called having stateChangeMutex acquired.
     **/
    virtual global_bytesize swapIn ( managedMemoryChunk *chunk ) = 0;
    /** @brief Mark chunk as deleted
     *  @note this is only called when deleting a chunk in managedMemory if chunk->swapBuf is not NULL
     *  @note this function must be called having stateChangeMutex acquired.
     **/
    virtual void swapDelete ( managedMemoryChunk *chunk ) = 0;

    ///Simple getter
    virtual inline global_bytesize getSwapSize() const {
        return swapSize;
    }
    ///Simple getter
    virtual inline global_bytesize getUsedSwap() const {
        return swapUsed;
    }
    ///Simple getter
    virtual inline global_bytesize getFreeSwap() const {
        return swapFree;
    }
    ///Returns possible memory alignment restrictions
    inline size_t getMemoryAlignment() const {
        return memoryAlignment;
    }
    /** @brief account for memory usage change
     *  @param bytes number of bytes under consideration
     *  @param rambytes set this to true if you want to signal different usage for bytes residing in rambytes
     *  @param used sum of bytes will be increased if true, else decreased.
     *  @note this function must be called having stateChangeMutex acquired.**/
    void claimUsageof ( global_bytesize bytes, bool rambytes, bool used );
    /** @brief Function waits for all asynchronous IO to complete.
      * The wait is implemented non-performant as a normal user does not have to wait for this.
      * Implementing this with a _cond just destroys performance in the respective swapIn/out procedures without increasing any user space functionality.
      **/
    void waitForCleanExit();

    virtual inline bool checkForAIO() {
        return false;
    }

    //Caching Control.
    /** @brief throws out cached elements still in ram but also resident on disk. This makes space in situations of low swap memory
     * @note this function must be called having stateChangeMutex acquired.
     **/
    virtual inline bool cleanupCachedElements ( rambrain::global_bytesize minimum_size = 0 ) {
        return true;
    }
    /** @brief tells managedFileSwap that the chunk under consideration might have been changed by user and needs to be copied out freshly
     * @note this function must be called having stateChangeMutex acquired.
     **/
    virtual inline void invalidateCacheFor ( managedMemoryChunk &chunk ) {}

protected:
    global_bytesize swapSize;
    global_bytesize swapUsed;
    global_bytesize swapFree;
    unsigned int totalSwapActionsQueued = 0;

    size_t memoryAlignment = 1;
};


}




#endif







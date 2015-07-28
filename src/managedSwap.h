/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MANAGEDSWAP_H
#define MANAGEDSWAP_H

#include "managedMemoryChunk.h"
#include "common.h"
#include "managedMemory.h"
#include "rambrain_atomics.h"
#include "configreader.h"

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


    /** @brief extend swap by policy
     * @return success
     * @param min_size Minimum size the swap has to be extended before success is returned
     **/
    virtual bool extendSwapByPolicy ( global_bytesize min_size ) {
        errmsg ( "Your swap module does not support extending swap by policy" );
        return false;
    };
    /** @brief extend swap by size number of bytes
     * @return success
     **/
    virtual bool extendSwap ( global_bytesize size ) {
        errmsg ( "Your swap module does not support extending swap" );
        return false;
    }

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

    //Simple getter
    virtual inline swapPolicy getSwapPolicy() const {
        return policy;
    }

    //Simple setter returning old policy
    virtual inline swapPolicy setSwapPolicy ( swapPolicy newPolicy ) {
        swapPolicy oldPolicy = policy;
        policy = newPolicy;
        return oldPolicy;
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

    swapPolicy policy = swapPolicy::fixed;
};


}




#endif








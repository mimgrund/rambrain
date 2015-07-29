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

#ifndef MANAGEDMEMORY_H
#define MANAGEDMEMORY_H

#include <stdlib.h>
#include <map>
#include <pthread.h>

#ifdef SWAPSTATS
#include <signal.h>
#ifdef LOGSTATS
#include <chrono>
#include "timer.h"
#endif
#endif

#include "managedMemoryChunk.h"
#include "exceptions.h"


//Test classes
#ifdef BUILD_TESTS
class managedFileSwap_Unit_ManualSwapping_Test;
class managedFileSwap_Unit_ManualSwappingDelete_Test;
class managedFileSwap_Unit_ManualMultiSwapping_Test;
#endif

namespace rambrain
{
class managedFileSwap;
class managedSwap;
template<class T>
class managedPtr;
/**
 * @brief Backend class to handle raw memory and interaction/storage with managedSwap.
 *
 * @note Different strategies which elements to swap out / in can be implemented by inheriting this class
 * @note End users do not need to instantiate/use this class directly. This will be done by config manager
 *
 * This class's implementation is written to standardize most of interactions between swap and managedMemory derived classes.
 * Implementation supports asynchronous actions. While there may be more parallelism possible we currently
 * support only one mutex ( stateChangeMutex ) to make this thread-safe to keep things simple at the moment.
 * @warning When writing new strategies, study the locking/unlocking of stateChangeMutex closely to prevent deadlocks. ManagedFileSwap may serve as an example.
 *
 * @warning Only the most recent allocated memoryManager will be used by adhereTo / managedPtr classes
 */
class managedMemory
{
public:
    /**
    * @brief Standard constructor
    * @param swap The swap that this manager should use.
    * @param size The net size of memory that is allowed in allocation. Be aware of leaving enough space for overhead.
    **/
    managedMemory ( managedSwap *swap, global_bytesize size = gig );
    virtual ~managedMemory();

    //Memory Management options
    /** @brief dynamically adjusts allowed ram usage
     *  @param size desired allowed ram size
     *  @return returns whether the new restrictions could be fulfilled under current load
     *  If necessary, this function will trigger writeout of objects until the new limit is reached.
     **/
    bool setMemoryLimit ( global_bytesize size );
    /// @brief returns current memory limit
    global_bytesize getMemoryLimit () const;
    /// @brief returns current ram usage
    global_bytesize getUsedMemory() const;
    /// @brief returns current swap usage
    global_bytesize getSwappedMemory() const;
    /** @brief set policy what to do when out of memory in both ram and swap
     * @return old policy
     **/
    bool setOutOfSwapIsFatal ( bool fatal = true );


    //Chunk Management
    ///Triggers swapin of chunk
    bool prepareUse ( rambrain::managedMemoryChunk &chunk, bool acquireLock = true );
    /// Convenience interface for setUse( managedMemoryChunk &chunk, bool writeAccess ) @see setUse(managedMemoryChunk &chunk,bool writeAccess);
    bool setUse ( memoryID id );
    /// Convenience interface for unsetUse ( managedMemoryChunk &chunk, bool writeAccess ) @see unsetUse(managedMemoryChunk &chunk,bool writeAccess);
    bool unsetUse ( memoryID id );
    /** @brief Marks chunk as used and prevents swapout
     * @return success
     * @param chunk the chunk that will be used
     * @param writeAccess set this to false if the chunk will not be written to
     **/
    bool setUse ( managedMemoryChunk &chunk, bool writeAccess );
    /** @brief Marks chunk as unused again.
     *  @return success
     *  @param chunk the chunk that will not be needed in near future
     *  @param no_unsets if you set use to the chunk n times, you may set this to n instead of calling n times**/
    bool unsetUse ( managedMemoryChunk &chunk , unsigned int no_unsets = 1 );


#ifdef PARENTAL_CONTROL
    //Tree Management
    /// @brief conveniently returns number of children of the memoryChunk with id id
    unsigned int getNumberOfChildren ( const memoryID &id );
    /// @brief prints the tree of managed objects for further inspection
    void printTree ( managedMemoryChunk *current = NULL, unsigned int nspaces = 0 );

    static const memoryID root;
    static memoryID parent;
    //We need the following two to correctly initialize class hierarchies:
    static bool threadSynced;
    static pthread_mutex_t parentalMutex;
    static pthread_cond_t parentalCond;
    static pthread_t creatingThread;
    /// @brief recursively deletes the objects in memory, first children, then parents.
    void recursiveMfree ( memoryID id );
#else
    /// @brief linearly deletes all objects in memory
    void linearMfree();
#endif
    static managedMemory *defaultManager;
    static const memoryID invalid;
    /** @brief signals that a swapping action has completed and memory limits have changed
     *  @note this function is used e.g. by managedSwap to give other threads waiting the possibility to check whether the condition they're waiting for is met now.
     * **/

    static void signalSwappingCond();
protected:
    /// @brief allocates and registers a new raw memory chunk of size sizereq to be filled in by managedPtr
    managedMemoryChunk *mmalloc ( global_bytesize sizereq );
    /// @brief this function is a stub. In the future it should be capable of resizing an existing allocation
    bool mrealloc ( memoryID id, global_bytesize sizereq );
    /// @brief this function unregisters and deallocates a chunk
    void mfree ( rambrain::memoryID id, bool inCleanup = false );
    ///returns a reference to the memoryChunk indexed by id id
    managedMemoryChunk &resolveMemChunk ( const memoryID &id );


    ///Error codes for swapOut requests
    enum swapErrorCode {
        ///Swapping action was successful
        ERR_SUCCESS,
        ///Swap space is full
        ERR_SWAPFULL ,
        ///The element/size requested does not fit in RAM as a whole
        ERR_MORETHANTOTALRAM,
        ///We lack reasonable candidates for swapout (too much elements in use?)
        ERR_NOTENOUGHCANDIDATES
    };
    /** @brief swaps out at least min_size bytes
     *  @return returns whether the swap out was successful or a reason why it was note
     *  @param min_size minimum size of swapped out elements
     *  Sucessful return does not mean that the bytes are available at an instant. It may be necessary for swapOut to complete beforehands.
     *  @note this function must be called having stateChangeMutex acquired.
     *
     **/
    virtual swapErrorCode swapOut ( global_bytesize min_size ) = 0;
    /// @brief Convenience function for swapIn ( managedMemoryChunk &chunk ) @see swapIn(managedMemoryChunk &chunk)
    virtual bool swapIn ( memoryID id );
    /** @brief Tries to swap in chunk chunk
     *  @return success
     *  @param chunk the chunk subject to be swapped in
     *  Successful return does not mean that the chunk is available at an instant. It may be necessary for asynchronous swapIn to complete beforehands.
     *  @note this function must be called having stateChangeMutex acquired.
    **/
    virtual bool swapIn ( managedMemoryChunk &chunk ) = 0;
    /** @brief marks chunk as recently active as a hint for scheduling **/
    virtual bool touch ( managedMemoryChunk &chunk ) = 0;
    ///@brief gives scheduler code the opportunity to register its own datastructures associated with a chunk
    virtual void schedulerRegister ( managedMemoryChunk &chunk ) = 0;
    ///@brief signals deletion of chunk to scheduler code
    virtual void schedulerDelete ( managedMemoryChunk &chunk ) = 0;

    /** @brief This function ensures that there is sizereq space left in ram
        @param orisSwappedin if not null, this chunk will be checked for ram presence
        @return If \p orisSwappedin is set, return value tells whether the chunk \p orisSwappedin is pointing to has been or is about to be swapped in. Otherwise false**/

    bool ensureEnoughSpace ( global_bytesize sizereq, managedMemoryChunk *orIsSwappedin = NULL );

    //Swap Storage manager iface:
    managedSwap *swap = 0;

    global_bytesize memory_max; //1GB
    global_bytesize memory_used = 0;
    global_bytesize memory_swapped = 0;
    global_bytesize memory_tobefreed = 0;
    bool outOfSwapIsFatal = true;

    std::map<memoryID, managedMemoryChunk *> memChunks;

    memoryAtime atime = 0;
    memoryID memID_pace = 1;


    managedMemory *previousManager;
    /// Custom throw function, as we need to prevent throwing exceptions in construtors.
    static bool Throw ( memoryException e );

    static pthread_mutex_t stateChangeMutex;
    //Signalled after every swapin. Synchronization is happening via stateChangeMutex
    static pthread_cond_t swappingCond;
    /*static pthread_cond_t topologicalCond;*/

    template<class T>
    friend class managedPtr;
    friend class managedSwap;
    friend class managedFileSwap;
    //Test classes
#ifdef BUILD_TESTS
    friend class ::managedFileSwap_Unit_ManualSwapping_Test;
    friend class ::managedFileSwap_Unit_ManualMultiSwapping_Test;
    friend class ::managedFileSwap_Unit_ManualSwappingDelete_Test;
    friend managedSwap *configTestGetSwap ( managedMemory *man );
#endif

#ifdef SWAPSTATS
protected:
    global_bytesize n_swap_out = 0;
    global_bytesize n_swap_in = 0;


    global_bytesize swap_out_scheduled_bytes = 0;
    global_bytesize swap_in_scheduled_bytes = 0;
    global_bytesize swap_out_bytes = 0;
    global_bytesize swap_in_bytes = 0;

    global_bytesize swap_out_bytes_last = 0;
    global_bytesize swap_in_bytes_last = 0;

    global_bytesize swap_hits = 0;
    global_bytesize swap_misses = 0;
    /** @brief Waits until a certain chunk is present
     *  @return success
     *  @warning ensure that chunk is to be swapped in or may be present.
     *  @note this function must be called having stateChangeMutex acquired.**/
    bool waitForSwapin ( managedMemoryChunk &chunk, bool keepSwapLock = false );
    /** @brief Waits until a certain chunk is swapped out
     *  @return success
     *  @warning ensure that chunk is to be swapped out in or may have already been swapped.
     *  @note this function must be called having stateChangeMutex acquired.**/
    bool waitForSwapout ( managedMemoryChunk &chunk, bool keepSwapLock = false );
    /** @brief account for memory usage change
     *  @param bytes number of bytes under consideration
     *  @param rambytes set this to true if you want to signal different usage for bytes residing in rambytes
     *  @param used sum of bytes will be increased if true, else decreased.
    @note this function must be called having stateChangeMutex acquired.**/
    void claimUsageof ( global_bytesize bytes, bool rambytes, bool used );
    /** @brief account for future availability of bytes
     *  @param bytes number of bytes under consideration
     *  @param tobefreed sum of bytes will be increased if true, else decreased.
     *  @note this function must be called having stateChangeMutex acquired.
     *  This tells memoryManager that asynchronous actions pending will free the number of bytes given by parameter
     *  or that these bytes have now been freed
     **/
    void claimTobefreed ( global_bytesize bytes, bool tobefreed );
    /** @brief wait for some asynchronous action to occur
        @note this function must be called having stateChangeMutex acquired.**/
    void waitForAIO();

    size_t memoryAlignment = 1;
#ifdef LOGSTATS
    //! @warning This will induce a serious issue in combination with MPI and a shared disk. How to handle that case?
    static FILE *logFile;
    static bool firstLog;
#endif

public:
    ///@brief print statistic about the number, size and efficiency of swapping actions
    void printSwapstats() const;
    ///@brief reset statistic about the number, size and efficiency of swapping actions
    void resetSwapstats();
    /**@brief static binding that will print out some stats.
    Compile with cmake -DSWAPSTATS=on and send process SIGUSR1 to call this function
    **/
    static void sigswapstats ( int sig );
#endif
    ///@brief prints out a GIT version info and a diff on this version at compile time
    static void versionInfo();
};

}

#endif



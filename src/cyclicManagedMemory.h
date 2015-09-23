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

#ifndef CYCLICMANAGEDMEMORY_H
#define CYCLICMANAGEDMEMORY_H

#include "managedMemory.h"

namespace rambrain
{
///@brief structure created by scheduler to track access times of memoryChunks
struct cyclicAtime {
    managedMemoryChunk *chunk;///The chunk
    cyclicAtime *next; ///Next chunk in cycle
    cyclicAtime *prev;///Prev chunk in cycle
};


enum backlog_action {UNKNOWN, REGISTER, DELETE, SWAPOUT, SWAPIN, DECAY, TOUCH, CHECK};

union backlog_value {
    memoryID id;
    global_bytesize size;
};
struct backlog_entry {
    backlog_action action = UNKNOWN;
    backlog_value value;
};




///@brief scheduler working with a double linked cycle. Details see paper.
class cyclicManagedMemory : public managedMemory
{
public:
    cyclicManagedMemory ( rambrain::managedSwap *swap, rambrain::global_bytesize size );
    virtual ~cyclicManagedMemory();

    ///@brief prints out all elements known to the scheduler with their respective status
    void printCycle() const;
    ///@brief prints out current memory usage
    void printMemUsage() const;
    /** @brief checks whether the scheduler believes to be sane and prints an error message if not
     *  @return true if sane, false if not**/
    bool checkCycle();

    /**
     * @brief sets whether scheduler should guess next needed items
     * @param preemtive iff set to true, scheduler will guess (default)
     * \note not thread-safe - does not make sense to call it from different threads anyway
     * @return previous value
     */
    bool setPreemptiveLoading ( bool preemptive );
    /**
     * @brief sets whether scheduler should swap out elements without strict need
     * @param preemtive iff set to true, scheduler will guess (default)
     * \note not thread-safe - does not make sense to call it from different threads anyway
     * @return previous value
     */
    bool setPreemptiveUnloading ( bool preemptive );

    struct chain {
        cyclicAtime *from, *to;
    };



private:
    /**
     * @brief cyclic implementation of swapIn, see paper
     * @note protect call to swapIn by topologicalMutex
     */
    virtual bool swapIn ( managedMemoryChunk &chunk );
    /**
     * @brief cyclic implementation of swapOut, see paper
     * @note protect call to swapIn by topologicalMutex
     */
    virtual swapErrorCode swapOut ( global_bytesize min_size );
    virtual bool touch ( managedMemoryChunk &chunk );
    ///@brief tries to regulate immediately usable free memory in ram to a level optimal for preemptive loading
    virtual void untouch ( managedMemoryChunk &chunk );
    virtual void schedulerRegister ( managedMemoryChunk &chunk );
    virtual void schedulerDelete ( managedMemoryChunk &chunk );
    ///@brief: Tries to unload around bytes bytes of preemptive elements
    void decay ( global_bytesize bytes );


    //loop pointers:
    cyclicAtime *active = NULL;
    cyclicAtime *counterActive = NULL;
    cyclicAtime *preemptiveStart = NULL;

    float swapOutFrac = .8;
    float swapInFrac = .9;

    bool preemtiveSwapIn = true;
    bool preemtiveSwapOut = true;
    global_bytesize preemptiveBytes = 0;
    unsigned int consecutivePreemptiveTransactions = 0;
    unsigned int preemptiveSinceLast = 0;

    static pthread_mutex_t cyclicTopoLock;

    double preemptiveTurnoffFraction = .01;


    ///@brief: separates all chunks matching state in list separateStatus  or preeemptiveLoaded from the ring
    struct cyclicManagedMemory::chain filterChain ( chain &toFilter, const memoryStatus *separateStatus, bool *preemptiveLoaded = NULL );
    static void insertBefore ( cyclicAtime *pos, chain separated );
    static void printChain ( const char *name, const chain mchain );

    void printBacklog() const;
    static const unsigned int backlog_size = 100;
    backlog_entry backlog[backlog_size];
    unsigned int backlog_pos = 0;

};

#define MUTUAL_CONNECT(A,B) A->next = B; B->prev = A;

#define BACKLOG_ADD_SIZE(maction,mvalue) {backlog[backlog_pos].action  = maction;backlog[backlog_pos].value.size= mvalue;backlog_pos=++backlog_pos%backlog_size;};
#define BACKLOG_ADD_ID(maction,mvalue) {backlog[backlog_pos].action  = maction;backlog[backlog_pos].value.id= mvalue ;backlog_pos=++backlog_pos%backlog_size;};


}

#endif


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
    bool checkCycle() const;

    /**
     * @brief sets whether scheduler should guess next needed items
     * @param preemtive iff set to true, scheduler will guess (default)
     * \note not thread-safe - does not make sense to call it from different threads anyway
     * \note returns previous value
     * @return returns previous value
     */
    bool setPreemptiveLoading ( bool preemptive );



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
    virtual void schedulerRegister ( managedMemoryChunk &chunk );
    virtual void schedulerDelete ( managedMemoryChunk &chunk );

    //loop pointers:
    cyclicAtime *active = NULL;
    cyclicAtime *counterActive = NULL;

    float swapOutFrac = .8;
    float swapInFrac = .9;

    bool preemtiveSwapIn = true;
    global_bytesize preemptiveBytes = 0;

    static pthread_mutex_t cyclicTopoLock;

    double preemptiveTurnoffFraction = .01;


};

#define MUTUAL_CONNECT(A,B) A->next = B; B->prev = A;

}

#endif


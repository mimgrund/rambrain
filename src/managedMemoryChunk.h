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

#ifndef MANAGEDMEMORYCHUNK_H
#define MANAGEDMEMORYCHUNK_H

#include "common.h"

namespace rambrain
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
 * @note when changing status, you have to hold the stateChangeMutex of the associated memoryManager
 * @warning There may be changes to any objects status when calling waiting functions in managedMemory or managedSwap.
 * Thus the user has to check the status of the object again having called such functions or having freshly acquired the lock
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
    memoryStatus status ;
    unsigned short useCnt /** @brief Number of using adhereTos or a possible location for locking the object to changes **/;
    void *locPtr /** @brief pointer to the actual data in RAM **/;
    global_bytesize size /** @brief Size of actual object in bytes**/;

    //Organization
    memoryID id /** @brief an ID to identify the object in scheduler or elsewhere **/;
#ifdef PARENTAL_CONTROL
    memoryID parent /** @brief parent element if created in class hierarchy **/;
    memoryID next/** @brief next element**/;
    memoryID child /** @brief first child element if creating a class hierarchy **/;
#endif
    bool preemptiveLoaded = false;

    //Swap scheduling:
    void *schedBuf /** @brief a place to store additional scheduling information **/;

    //Swap raw management:
    void *swapBuf/** @brief a place to store additional swapping information **/;
};

}

#endif


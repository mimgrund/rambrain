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

#ifndef DUMMYMANAGEDMEMORY_H
#define DUMMYMANAGEDMEMORY_H

#include "managedMemory.h"
#include "managedDummySwap.h"
#include "exceptions.h"

namespace rambrain
{

/** @brief a dummy managed Memory that basically does nothing and throws on everything.
 *  @note there is no productive use of this class, it only serves testing purposes
 **/
class dummyManagedMemory : public managedMemory
{
public:
    dummyManagedMemory() : managedMemory ( new managedDummySwap ( 0 ), 0 ) {}
    virtual ~dummyManagedMemory() {
        delete swap;
    }

protected:
    inline virtual swapErrorCode swapOut ( global_bytesize ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
        return ERR_SUCCESS;
    }
    inline virtual bool swapIn ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
        return ERR_SUCCESS;
    }
    inline virtual bool touch ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
        return ERR_SUCCESS;
    }
    inline virtual void schedulerRegister ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
    }
    inline virtual void schedulerDelete ( managedMemoryChunk & ) {
        pthread_mutex_unlock ( &managedMemory::stateChangeMutex );
        Throw ( memoryException ( "No memory manager in place." ) );
    }

};

}

#endif // DUMMYMANAGEDMEMORY_H








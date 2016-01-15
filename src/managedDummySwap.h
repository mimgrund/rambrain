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

#ifndef MANAGEDDUMMYSWAP_H
#define MANAGEDDUMMYSWAP_H

#include "managedMemoryChunk.h"
#include "managedSwap.h"

namespace rambrain
{
/** @brief A dummy swap that just copies swapped out chunks to a different location in ram
 *  @note there is no productive use of this class, it simly serves testing purpuses for the managedMemory derivated classes
 **/
class managedDummySwap : public managedSwap
{
public:
    managedDummySwap ( rambrain::global_bytesize size );
    virtual ~managedDummySwap() {
        close();
    }

    virtual global_bytesize swapIn ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapIn ( managedMemoryChunk *chunk );
    virtual global_bytesize swapOut ( managedMemoryChunk **chunklist, unsigned int nchunks );
    virtual global_bytesize swapOut ( managedMemoryChunk *chunk );
    virtual void swapDelete ( managedMemoryChunk *chunk );

    virtual void close() {
        closed = true;
    }
};

}

#endif


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

#include "managedMemoryChunk.h"
#include <stdlib.h>

namespace rambrain
{

#ifdef PARENTAL_CONTROL
managedMemoryChunk::managedMemoryChunk ( const memoryID &parent, const memoryID &me ) :
    useCnt ( 0 ),     parent ( parent ), id ( me ), swapBuf ( NULL )
{
}
#else
managedMemoryChunk::managedMemoryChunk (  const memoryID &me ) :
    useCnt ( 0 ), id ( me ), swapBuf ( NULL )
{
}

#endif
}
#include "managedMemoryChunk.h"
#include <stdlib.h>

namespace membrain
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
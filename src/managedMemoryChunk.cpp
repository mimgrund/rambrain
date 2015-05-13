#include "managedMemoryChunk.h"
#include <stdlib.h>

namespace membrain
{

managedMemoryChunk::managedMemoryChunk ( const memoryID &parent, const memoryID &me )
    : useCnt ( 0 ), parent ( parent ), id ( me ), swapBuf ( NULL )
{
}

}

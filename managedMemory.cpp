#include "managedMemory.h"
#include "common.h"

managedMemory::managedMemory(unsigned int size){
  memory_max = size;
  if(!defaultManager)
    defaultManager = this;
  managedMemoryChunk *chunk = mmalloc(0,0); //Create root element.
  chunk->status = MEM_ROOT; 
}

unsigned int managedMemory::getMemoryLimit(unsigned int size)
{
  return memory_max;
}

bool managedMemory::setMemoryLimit(unsigned int size)
{
  return false; //Resetting this is not implemented yet.
}

managedMemoryChunk* managedMemory::mmalloc(unsigned int sizereq, const memoryID& parent)
{
  if(sizereq+memory_used>memory_max){
    if(!swapOut(sizereq)){
      errmsgf("Could not swap out >%d Bytes\nOut of Memory.",sizereq);
      throw;
    }
  }
  //We are left with enough free space to malloc.
  managedMemoryChunk *chunk = new managedMemoryChunk(parent,memID_pace++);
  chunk->status = MEM_ALLOCATED;
  chunk->locPtr = malloc(sizereq);
  if(!chunk->locPtr){
    errmsgf("Classical malloc on a size of %d failed.",sizereq);
    throw;
  }
  chunk->atime = atime++;
  memory_used += sizereq;
  chunk->size = sizereq;
  chunk->child = 0;
  chunk->parent = 0;
  if(memChunks.size()==0){//We're inserting root elem.
    chunk->next =0;
  }else{
    //fill in tree:
    managedMemoryChunk pchunk = resolveMemChunk(parent);
    if(pchunk.child == 0){
      pchunk.child = chunk->id;
    }else{
      pchunk.child = chunk->id;
      chunk->next = pchunk.child;
    }
    
  }
  
  memChunks.insert({chunk->id,chunk});
  return chunk;
}

bool managedMemory::swapOut(unsigned int min_size)
{//TODO: Implement swapping strategy
  return false;
}
bool managedMemory::swapIn(managedMemoryChunk& chunk)
{
  switch(chunk.status){
    case MEM_ROOT:
      return false;
    case MEM_SWAPPED:
      errmsg("Swapping not implemented yet");
      return false;
      
  }
  return true;
}


bool managedMemory::swapIn(memoryID id)
{
  managedMemoryChunk chunk = resolveMemChunk(id);
  return swapIn(chunk);
}

bool managedMemory::setUse(managedMemoryChunk& chunk)
{
  switch(chunk.status){
    case MEM_ALLOCATED_INUSE:
      infomsg("Chunk already in use! Double importing not supported yet");
      return true;
    
    case MEM_ALLOCATED:
      chunk.status = MEM_ALLOCATED_INUSE;
      chunk.atime = atime++;
      return true;
    case MEM_SWAPPED:
      if(swapIn(chunk))
	return setUse(chunk);
      else
	return false;
      
  }
  return false;
}

bool managedMemory::unsetUse(memoryID id)
{
  managedMemoryChunk chunk = resolveMemChunk(id);
  return unsetUse(chunk);
}


bool managedMemory::setUse(memoryID id)
{
  managedMemoryChunk chunk = resolveMemChunk(id);
  setUse(chunk);
}

bool managedMemory::unsetUse(managedMemoryChunk& chunk)
{
  if(chunk.status==MEM_ALLOCATED_INUSE){
    chunk.status = MEM_ALLOCATED;
    return true;
  }else{
    throw;
  }
}


void managedMemory::mfree(memoryID id)
{
  managedMemoryChunk * chunk = memChunks[id];
  if(chunk->status==MEM_ALLOCATED_INUSE){
    errmsg("Trying to free memory that is in use.");
    return;
  }
  if(chunk->child!=-1){
    errmsg("Trying to free memory that has still alive children.");
    return;
  }
    
  if(chunk){
    
    if(chunk->status==MEM_ALLOCATED)
      free(chunk->locPtr);
    //get rid of hierarchy:
    if(chunk->id!=root){
	managedMemoryChunk* pchunk = &resolveMemChunk(chunk->parent);
	if(pchunk->child == chunk->id)
	  pchunk->child = 0;
	else{
	  pchunk = &resolveMemChunk(pchunk->child);
	  do{
	    pchunk = &resolveMemChunk(pchunk->next);
	    if(pchunk->next == chunk->id){
	      pchunk->next = chunk->next;
	    };
	  }while(pchunk->next!=0);  
	}
    }
    
    //Delete element itself
    memChunks.erase(id);    
    delete chunk;
  }
}



managedMemory* managedMemory::defaultManager=NULL;
memoryID managedMemory::root=0;
//memoryChunk class:

managedMemoryChunk::managedMemoryChunk(const memoryID& parent, const memoryID& me)
: parent(parent),id(me)
{
}

managedMemoryChunk& managedMemory::resolveMemChunk(const memoryID& id)
{
  return *(memChunks[id]);
}


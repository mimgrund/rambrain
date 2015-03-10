#include "managedMemory.h"
#include "common.h"

managedMemory::managedMemory(unsigned int size){
  memory_max = size;
  defaultManager = this;
  managedMemoryChunk *chunk = mmalloc(0); //Create root element.
  chunk->status = MEM_ROOT; 
}

managedMemory::~managedMemory()
{
  if(defaultManager==this)
    defaultManager=NULL;
  //Clean up objects:
  recursiveMfree(root);
  
  
}


unsigned int managedMemory::getMemoryLimit(unsigned int size) const
{
  return memory_max;
}
unsigned int managedMemory::getUsedMemory() const
{
  return memory_used;
}


bool managedMemory::setMemoryLimit(unsigned int size)
{
  return false; //Resetting this is not implemented yet.
}

managedMemoryChunk* managedMemory::mmalloc(unsigned int sizereq)
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
  chunk->child = invalid;
  chunk->parent = parent;
  if(chunk->id==root){//We're inserting root elem.
    chunk->next =invalid;
  }else{
    //fill in tree:
    managedMemoryChunk &pchunk = resolveMemChunk(parent);
    if(pchunk.child == invalid){
      chunk->next = invalid;//Einzelkind
      pchunk.child = chunk->id;
    }else{
      chunk->next = pchunk.child;
      pchunk.child = chunk->id;
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
    case MEM_ALLOCATED:
    case MEM_ALLOCATED_INUSE:
      warnmsg("Trying to swap back sth already in RAM");
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
    case MEM_ROOT:
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
  return setUse(chunk);
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
  if(chunk->child!=invalid){
    errmsg("Trying to free memory that has still alive children.");
    return;
  }
    
  if(chunk){
    
    if(chunk->status==MEM_ALLOCATED)
      free(chunk->locPtr);
    memory_used-= chunk->size;
    //get rid of hierarchy:
    if(chunk->id!=root){
	managedMemoryChunk* pchunk = &resolveMemChunk(chunk->parent);
	if(pchunk->child == chunk->id)
	  pchunk->child = chunk->next;
	else{
	  pchunk = &resolveMemChunk(pchunk->child);
	  do{
	    if(pchunk->next == chunk->id){
	      pchunk->next = chunk->next;
	    };
	  }while(pchunk->next!=invalid);  
	}
    }
    
    //Delete element itself
    memChunks.erase(id); 
    delete chunk;
  }
}

void managedMemory::recursiveMfree(memoryID id)
{
  managedMemoryChunk *oldchunk = &resolveMemChunk(id);
  managedMemoryChunk *next;
  do{
    if(oldchunk->child!=invalid)
      recursiveMfree(oldchunk->child);
    if(oldchunk->next!=invalid)
      next= &resolveMemChunk(oldchunk->child);
    else
      break;
    mfree(oldchunk->id);
    oldchunk = next;
  }while(1==1);
}


managedMemory* managedMemory::defaultManager=NULL;
memoryID const managedMemory::root=1;
memoryID const managedMemory::invalid=0;
memoryID managedMemory::parent=1;


unsigned int managedMemory::getNumberOfChildren(const memoryID& id)
{
  const managedMemoryChunk &chunk = resolveMemChunk(id);
  if(chunk.child==invalid)
    return 0;
  unsigned int no=1;
  const managedMemoryChunk *child = &resolveMemChunk(chunk.child);
  while(child->next!=invalid){
    child = &resolveMemChunk(child->next);
    no++;
  }
  return no;
}

void managedMemory::printTree(managedMemoryChunk *current,unsigned int nspaces)
{
  if(!current)
    current = &resolveMemChunk(root);
  do{
    for(unsigned int n=0;n<nspaces;n++)
      printf("  ");
    printf("(%d : size %d Bytes, atime %d, ",current->id,current->size,current->atime);
    switch(current->status){
      case MEM_ROOT:
	printf("Root Element");
	break;
      case MEM_SWAPPED:
	printf("Swapped out");
	break;
      case MEM_ALLOCATED:
	printf("Allocated");
	break;
      case MEM_ALLOCATED_INUSE:
	printf("Allocated&inUse");
	break;
    }
    printf(")\n");
    if(current->child!=invalid)
      printTree(&resolveMemChunk(current->child),nspaces+1);
    if(current->next!=invalid)
      current = &resolveMemChunk(current->next);
    else
      break;;
  }while(1==1);
}

//memoryChunk class:

managedMemoryChunk::managedMemoryChunk(const memoryID& parent, const memoryID& me)
: parent(parent),id(me)
{
}

managedMemoryChunk& managedMemory::resolveMemChunk(const memoryID& id)
{
  return *memChunks[id];
}


#ifndef MANAGEDMEMORY_H
#define MANAGEDMEMORY_H
#include <stdlib.h>
#include <map>

template<class T>
class managedPtr;

enum memoryStatus {MEM_ALLOCATED_INUSE,
		 MEM_ALLOCATED,
		 MEM_SWAPPED,
		 MEM_ROOT
};

typedef unsigned int memoryID;
typedef unsigned int memoryAtime;

class managedMemoryChunk{
public:
  managedMemoryChunk(const memoryID &parent, const memoryID &me);
  memoryStatus status;
  void * locPtr;
  unsigned int size;
  memoryID id;
  memoryID parent;
  memoryID next;
  memoryID child;
  
  memoryAtime atime;
};

class managedMemory{
public:
  managedMemory(unsigned int size=1073741824);
  bool setMemoryLimit(unsigned int size);
  unsigned int getMemoryLimit(unsigned int size);
  bool setUse(memoryID id);
  bool unsetUse(memoryID id);
  bool setUse(managedMemoryChunk &chunk);
  bool unsetUse(managedMemoryChunk &chunk);
  
  static managedMemory *defaultManager;
  static memoryID root;
private:
  managedMemoryChunk* mmalloc(unsigned int sizereq,const memoryID &parent);
  bool mrealloc(memoryID id,unsigned int sizereq);
  void mfree(memoryID id);
  managedMemoryChunk &resolveMemChunk(const memoryID &id);
  
  bool swapOut(unsigned int min_size);
  bool swapIn(memoryID id);
  bool swapIn(managedMemoryChunk &chunk);
  
  
  unsigned int memory_max; //1GB 
  unsigned int memory_used=0;
  
  memoryAtime atime=0;
  memoryID memID_pace=0;
  
  template<class T>
  friend class managedPtr;
  
  std::map<memoryID,managedMemoryChunk*> memChunks;
};

#endif



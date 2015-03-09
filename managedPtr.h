#ifndef MANAGEDPTR_H
#define MANAGEDPTR_H
#include "managedMemory.h"
#include "common.h"
template <class T>
class managedPtr{
public:
  managedPtr(memoryID parent, unsigned int n_elem){
    chunk = managedMemory::defaultManager->mmalloc(sizeof(T)*n_elem,parent);
  };
  bool setUse(){
    managedMemory::defaultManager->setUse(*chunk);
  }
  bool unsetUse(){
    managedMemory::defaultManager->unsetUse(*chunk);
  }
  
  ~managedPtr(){
    managedMemory::defaultManager->mfree(chunk->id);
  }
private:
  managedMemoryChunk *chunk;
  
  T* getLocPtr(){
    if(chunk->status==MEM_ALLOCATED_INUSE)
      return (T*) chunk->locPtr;
    else{
      errmsg("You have to sign Usage of the data first");
      return NULL;
    }
  };
  
  template<class G>
  friend class adhereTo;
};

template <class T>
class adhereTo{
public:
  adhereTo(managedPtr<T> &data, bool loadImidiately=false){
    this->data = &data;
    if(loadImidiately)
      data.setUse();
  };
  operator  T*(){
    data->setUse();
    return data->getLocPtr();
  };
  ~adhereTo(){
    data->unsetUse();
  };
  private:
  managedPtr<T> *data;
};

#endif
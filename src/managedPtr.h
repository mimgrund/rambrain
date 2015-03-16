#ifndef MANAGEDPTR_H
#define MANAGEDPTR_H
#include "managedMemory.h"
#include "common.h"

template <class T>
class managedPtr
{
public:
    managedPtr ( unsigned int n_elem ) {
        this->n_elem = n_elem;
        chunk = managedMemory::defaultManager->mmalloc ( sizeof ( T ) *n_elem );
        //Now call constructor and save possible children's sake:
        memoryID savedParent = managedMemory::parent;
        managedMemory::parent = chunk->id;
        setUse();
        for ( unsigned int n = 0; n<n_elem; n++ ) {
            new ( ( ( T* ) chunk->locPtr ) +n ) T();
        }
        unsetUse();
        managedMemory::parent = savedParent;
    };

    bool setUse() {
        return managedMemory::defaultManager->setUse ( *chunk );
    }
    bool unsetUse() {
        return managedMemory::defaultManager->unsetUse ( *chunk );
    }

    ~managedPtr() {
        for ( unsigned int n = 0; n<n_elem; n++ ) {
            ( ( ( T* ) chunk->locPtr ) +n )->~T();
        }
        managedMemory::defaultManager->mfree ( chunk->id );

    }
private:
    managedMemoryChunk *chunk;
    unsigned int n_elem;

    T* getLocPtr() {
        if ( chunk->status==MEM_ALLOCATED_INUSE ) {
            return ( T* ) chunk->locPtr;
        } else {
            errmsg ( "You have to sign Usage of the data first" );
            return NULL;
        }
    };

    template<class G>
    friend class adhereTo;
};


template <class T>
class adhereTo
{
public:
    adhereTo ( managedPtr<T> &data, bool loadImidiately=false ) {
        this->data = &data;
        if ( loadImidiately ) {
            loaded =data.setUse();

        }
    };
    operator  T*() {
        loaded = data->setUse();
        return data->getLocPtr();
    };
    ~adhereTo() {
        if(loaded)
            data->unsetUse();
    };
private:
    managedPtr<T> *data;
    bool loaded=false;
};


//Convenience macros
#define ADHERETO(class,instance) adhereTo<class> instance##_glue(instance);\
				 class* instance = instance##_glue;
#define ADHERETOLOC(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
				 class* locinstance = instance##_glue;



#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on;

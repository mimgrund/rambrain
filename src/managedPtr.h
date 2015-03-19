#ifndef MANAGEDPTR_H
#define MANAGEDPTR_H

#include "managedMemory.h"
#include "common.h"
#include "exceptions.h"

template <class T>
class managedPtr
{
public:
    managedPtr ( unsigned int n_elem ) {
        this->n_elem = n_elem;
        if ( n_elem == 0 ) {
            throw memoryException ( "Cannot allocate zero sized object." );
        }
        chunk = managedMemory::defaultManager->mmalloc ( sizeof ( T ) * n_elem );
        //Now call constructor and save possible children's sake:
        memoryID savedParent = managedMemory::parent;
        managedMemory::parent = chunk->id;
        setUse();
        for ( unsigned int n = 0; n < n_elem; n++ ) {
            new ( ( ( T * ) chunk->locPtr ) + n ) T();
        }
        unsetUse();
        managedMemory::parent = savedParent;
    }


    ~managedPtr() {
        bool oldthrow = managedMemory::defaultManager->noThrow;
        managedMemory::defaultManager->noThrow = true;
        for ( unsigned int n = 0; n < n_elem; n++ ) {
            ( ( ( T * ) chunk->locPtr ) + n )->~T();
        }
        managedMemory::defaultManager->mfree ( chunk->id );
        managedMemory::defaultManager->noThrow = oldthrow;
    }

    bool setUse ( bool writable = true ) {
        return managedMemory::defaultManager->setUse ( *chunk , writable );
    }

    bool unsetUse() {
        return managedMemory::defaultManager->unsetUse ( *chunk );
    }
    //!\TODO: smart Pointerize. at least copy constructor


private:
    managedMemoryChunk *chunk;
    unsigned int n_elem;

    T *getLocPtr() {
        if ( chunk->status == MEM_ALLOCATED_INUSE_WRITE ) {
            return ( T * ) chunk->locPtr;
        } else {
            throw unexpectedStateException ( "Can not get local pointer without setting usage first" );
            return NULL;
        }
    };

    const T *getConstLocPtr() {
        if ( chunk->status & MEM_ALLOCATED_INUSE_READ ) {
            return ( T * ) chunk->locPtr;
        } else {
            throw unexpectedStateException ( "Can not get local pointer without setting usage first" );
        }
    }

    template<class G>
    friend class adhereTo;

    // Test classes
    friend class managedPtr_Unit_ChunkInUse_Test;
    friend class managedPtr_Unit_GetLocPointer_Test;
};


template <class T>
class adhereTo
{
public:
    adhereTo ( managedPtr<T> &data, bool loadImidiately = false ) {
        this->data = &data;
        if ( loadImidiately ) {

            loaded = data.setUse ( true );

        }
    }
    operator  T *() {
        if ( !loaded || !loadedWritable ) {
            loaded = data->setUse ( true );
        }
        loadedWritable = true;
        return data->getLocPtr();
    };
    operator  const T *() {
        loaded = data->setUse ( false );

        return data->getConstLocPtr();
    };
    ~adhereTo() {
        if ( loaded ) {
            data->unsetUse();
        }
    }
private:
    managedPtr<T> *data;
    bool loaded = false;
    bool loadedWritable = false;

    // Test classes
    friend class adhereTo_Unit_LoadUnload_Test;
    friend class adhereTo_Unit_TwiceAdhered_Test;
};


//Convenience macros
#define ADHERETO(class,instance) adhereTo<class> instance##_glue(instance);\
				 class* instance = instance##_glue;
#define ADHERETOLOC(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
				 class* locinstance = instance##_glue;



#endif



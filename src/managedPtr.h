#ifndef MANAGEDPTR_H
#define MANAGEDPTR_H

#include "managedMemory.h"
#include "common.h"
#include "exceptions.h"

template <class T>
class adhereTo;


//Convenience macros
#define ADHERETO(class,instance) adhereTo<class> instance##_glue(instance);\
                 class* instance = instance##_glue;
#define ADHERETOCONST(class,instance) adhereTo<class> instance##_glue(instance);\
                 const class* instance = instance##_glue;
#define ADHERETOLOC(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
                 class* locinstance = instance##_glue;
#define ADHERETOLOCCONST(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
                 const class* locinstance = instance##_glue;


template <class T>
class managedPtr
{
public:
    managedPtr ( const managedPtr<T> &ref ) : tracker ( ref.tracker ), n_elem ( ref.n_elem ), chunk ( ref.chunk ) {
        ( *tracker )++;
    }

    managedPtr ( unsigned int n_elem ) {
        this->n_elem = n_elem;
        tracker = new unsigned int;
        ( *tracker ) = 0;
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
        ( *tracker ) = 1;
    }


    ~managedPtr() {
        -- ( *tracker );
        if ( *tracker  == 0 ) {
            mDelete();
        }
    }

    bool setUse ( bool writable = true ) {
        return managedMemory::defaultManager->setUse ( *chunk , writable );
    }

    bool unsetUse ( unsigned int loaded = 1 ) {
        return managedMemory::defaultManager->unsetUse ( *chunk , loaded );
    }

    managedPtr<T> &operator= ( const managedPtr<T> &ref ) {
        if ( ref.chunk == chunk ) {
            return *this;
        }
        if ( -- ( *tracker ) == 0 ) {
            mDelete();
        }
        n_elem = ref.n_elem;
        chunk = ref.chunk;
        tracker = ref.tracker;
        ++ ( *tracker );
        return *this;
    }

    DEPRECATED T &operator[] ( int i ) {
        managedPtr<T> &self = *this;
        ADHERETOLOC ( T, self, loc );
        return loc[i];
    }
    DEPRECATED const T &operator[] ( int i ) const {
        const managedPtr<T> &self = *this;
        ADHERETOLOC ( T, self, loc );
        return loc[i];
    }

private:
    managedMemoryChunk *chunk;
    unsigned int *tracker;
    unsigned int n_elem;

    void mDelete() {
        bool oldthrow = managedMemory::defaultManager->noThrow;
        managedMemory::defaultManager->noThrow = true;
        for ( unsigned int n = 0; n < n_elem; n++ ) {
            ( ( ( T * ) chunk->locPtr ) + n )->~T();
        }
        managedMemory::defaultManager->mfree ( chunk->id );
        managedMemory::defaultManager->noThrow = oldthrow;
        delete tracker;
    }

    T *getLocPtr() {
        if ( chunk->status == MEM_ALLOCATED_INUSE_WRITE ) {
            return ( T * ) chunk->locPtr;
        } else {
            throw unexpectedStateException ( "Can not get local pointer without setting usage first" );
            return NULL;
        }
    }

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
    friend class managedPtr_Unit_SmartPointery_Test;
};


template <class T>
class adhereTo
{
public:
    adhereTo ( managedPtr<T> &data, bool loadImidiately = false ) {
        this->data = &data;

        if ( loadImidiately ) {
            loaded = ( data.setUse ( true ) ? 1 : 0 );
            loadedWritable = true;
        }

    }
    operator  T *() {
        if ( loaded == 0 || !loadedWritable ) {
            loaded += ( data->setUse ( true ) ? 1 : 0 );
        }
        loadedWritable = true;
        return data->getLocPtr();
    }
    operator  const T *() {
        if ( loaded == 0 ) {
            loaded += ( data->setUse ( false ) ? 1 : 0 );
        }

        return data->getConstLocPtr();
    }
    ~adhereTo() {
        if ( loaded > 0 ) {
            loaded = ( data->unsetUse ( loaded ) ? 0 : loaded );
        }
    }
private:
    managedPtr<T> *data;
    unsigned int loaded = 0;
    bool loadedWritable = false;


    // Test classes
    friend class adhereTo_Unit_LoadUnload_Test;
    friend class adhereTo_Unit_LoadUnloadConst_Test;
    friend class adhereTo_Unit_TwiceAdhered_Test;
};

#endif

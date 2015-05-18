#ifndef MANAGEDPTR_H
#define MANAGEDPTR_H

#include "managedMemory.h"
#include "membrain_atomics.h"
#include "common.h"
#include "exceptions.h"
#include <type_traits>
#include <pthread.h>

// Test classes
class managedPtr_Unit_ChunkInUse_Test;
class managedPtr_Unit_GetLocPointer_Test;
class managedPtr_Unit_SmartPointery_Test;
class managedFileSwap_Unit_SwapSingleIsland_Test;
class managedFileSwap_Unit_SwapNextAndSingleIsland_Test;

class adhereTo_Unit_LoadUnload_Test;
class adhereTo_Unit_LoadUnloadConst_Test;
class adhereTo_Unit_TwiceAdhered_Test;

namespace membrain
{

template <class T>
class adhereTo;
template <class T>
class adhereToConst;


//Convenience macros
#define ADHERETO(class,instance) adhereTo<class> instance##_glue(instance);\
                 class* instance = instance##_glue;
#define ADHERETOCONST(class,instance) adhereToConst<class> instance##_glue(instance);\
                 const class* instance = instance##_glue;
#define ADHERETOLOC(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
                 class* locinstance = instance##_glue;
#define ADHERETOLOCCONST(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
                 const class* locinstance = instance##_glue;



/**
 * \brief Main class to allocate memory that is managed by the membrain memory defaultManager
 *
 * @warning _thread-safety_
 * * The object itself is not thread-safe
 * * Do not pass pointers/references to this object over thread boundaries
 * @note _thread-safety_
 * * The object itself may be passed over thread boundary
 *
 * **/
template <class T>
class managedPtr
{
public:
    managedPtr ( const managedPtr<T> &ref ) : tracker ( ref.tracker ), n_elem ( ref.n_elem ), chunk ( ref.chunk ) {

        membrain_atomic_add_fetch ( tracker, 1 );

    }

    template <typename... ctor_args>
    managedPtr ( unsigned int n_elem , ctor_args... Args ) {
        this->n_elem = n_elem;
        tracker = new unsigned int;
        ( *tracker ) = 1;
        if ( n_elem == 0 ) {
            return;
        }

        chunk = managedMemory::defaultManager->mmalloc ( sizeof ( T ) * n_elem );
        //Now call constructor and save possible children's sake:
        memoryID savedParent = managedMemory::parent;
        managedMemory::parent = chunk->id;
        setUse();
        for ( unsigned int n = 0; n < n_elem; n++ ) {
            new ( ( ( T * ) chunk->locPtr ) + n ) T ( Args... );
        }
        unsetUse();
        managedMemory::parent = savedParent;

        // before user can pass it around
    }


    ~managedPtr() {
        int trackerold = membrain_atomic_sub_fetch ( tracker, 1 );
        if ( trackerold  == 0 ) {//Ensure destructor to be called only once.
            mDelete<T>();
        }
    }

    bool setUse ( bool writable = true ) const {
        return managedMemory::defaultManager->setUse ( *chunk , writable );
    }

    bool unsetUse ( unsigned int loaded = 1 ) const {
        return managedMemory::defaultManager->unsetUse ( *chunk , loaded );
    }

    managedPtr<T> &operator= ( const managedPtr<T> &ref ) {
        if ( ref.chunk == chunk ) {
            return *this;
        }
        if ( membrain_atomic_sub_fetch ( tracker, 1 ) == 0 ) {
            mDelete<T>();
        }
        n_elem = ref.n_elem;
        chunk = ref.chunk;
        tracker = ref.tracker;
        membrain_atomic_add_fetch ( tracker, 1 );
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

    T *getLocPtr() {
        if ( chunk->status == MEM_ALLOCATED_INUSE_WRITE ) {
            return ( T * ) chunk->locPtr;
        } else {
            throw unexpectedStateException ( "Can not get local pointer without setting usage first" );
            return NULL;
        }
    }

    const T *getConstLocPtr() const {
        if ( chunk->status & MEM_ALLOCATED_INUSE_READ ) {
            return ( T * ) chunk->locPtr;
        } else {
            throw unexpectedStateException ( "Can not get local pointer without setting usage first" );
        }
    }

private:
    managedMemoryChunk *chunk;
    unsigned int *tracker;
    unsigned int n_elem;


    template <class G>
    typename std::enable_if<std::is_class<G>::value>::type
    mDelete (  ) {
        if ( n_elem > 0 ) {
            bool oldthrow = managedMemory::defaultManager->noThrow;
            managedMemory::defaultManager->noThrow = true;
            for ( unsigned int n = 0; n < n_elem; n++ ) {
                ( ( ( G * ) chunk->locPtr ) + n )->~G();
            }
            managedMemory::defaultManager->mfree ( chunk->id );
            managedMemory::defaultManager->noThrow = oldthrow;
        }
        delete tracker;
    }
    template <class G>
    typename std::enable_if < !std::is_class<G>::value >::type
    mDelete (  ) {
        if ( n_elem > 0 ) {
            bool oldthrow = managedMemory::defaultManager->noThrow;
            managedMemory::defaultManager->noThrow = true;
            managedMemory::defaultManager->mfree ( chunk->id );
            managedMemory::defaultManager->noThrow = oldthrow;
        }
        delete tracker;
    }






    template<class G>
    friend class adhereTo;
    template<class G>
    friend class adhereToConst;

    // Test classes
    friend class ::managedPtr_Unit_ChunkInUse_Test;
    friend class ::managedPtr_Unit_GetLocPointer_Test;
    friend class ::managedPtr_Unit_SmartPointery_Test;
    friend class ::managedFileSwap_Unit_SwapSingleIsland_Test;
    friend class ::managedFileSwap_Unit_SwapNextAndSingleIsland_Test;
};

/**
 * @brief Main class to fetch memory that is managed by membrain for actual usage.
 *
 * \warning _thread-safety_
 * * The object itself is not thread-safe
 * * Do not pass pointers/references to this object over thread boundaries
 *
 * \note _thread-safety_
 * * The object itself may be passed over thread boundaries
 * **/
template <class T>
class adhereTo
{
public:
    adhereTo ( const adhereTo<T> &ref ) {
        this->data = ref.data;
        if ( ref.loaded != 0 ) {
            loadedWritable = ref.loadedWritable;
            loaded = ref.loaded;
            for ( unsigned int nloaded = 0; nloaded < ref.loaded; ++nloaded ) {
                data->setUse ( loadedWritable );
            }

        } else {
            loadedWritable = false;
            loaded = 0;
        }

    };

    adhereTo ( managedPtr<T> &data, bool loadImidiately = false ) {
        this->data = &data;

        if ( loadImidiately ) {
            loaded = ( data.setUse ( true ) ? 1 : 0 );
            loadedWritable = true;
        }

    }

    adhereTo<T> &operator= ( const adhereTo<T> &ref ) {
        if ( loaded > 0 ) {
            loaded = ( data->unsetUse ( loaded ) ? 0 : loaded );
        }
        this->data = ref.data;
        if ( ref.loaded != 0 ) {
            loadedWritable = ref.loadedWritable;
            loaded = ref.loaded;
            for ( unsigned int nloaded = 0; nloaded < ref.loaded; ++nloaded ) {
                data->setUse ( loadedWritable );
            }

        } else {
            loadedWritable = false;
            loaded = 0;
        }
        return *this;
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
    friend class ::adhereTo_Unit_LoadUnload_Test;
    friend class ::adhereTo_Unit_LoadUnloadConst_Test;
    friend class ::adhereTo_Unit_TwiceAdhered_Test;
};

template <class T>
class adhereToConst
{
public:
    adhereToConst ( const adhereToConst<T> &ref ) {
        this->data = ref.data;
        if ( ref.loaded != 0 ) {
            loaded = ref.loaded;
            for ( unsigned int nloaded = 0; nloaded < ref.loaded; ++nloaded ) {
                data->setUse ( false );
            }

        } else {
            loaded = 0;
        }

    };

    adhereToConst ( const managedPtr<T> &data, bool loadImidiately = false ) {
        this->data = &data;

        if ( loadImidiately ) {
            loaded = ( data.setUse ( false ) ? 1 : 0 );
        }

    }

    adhereToConst<T> &operator= ( const adhereTo<T> &ref ) {
        if ( loaded > 0 ) {
            loaded = ( data->unsetUse ( loaded ) ? 0 : loaded );
        }
        this->data = ref.data;
        if ( ref.loaded != 0 ) {
            loaded = ref.loaded;
            for ( unsigned int nloaded = 0; nloaded < ref.loaded; ++nloaded ) {
                data->setUse ( false );
            }

        } else {
            loaded = 0;
        }
        return *this;
    }

    operator  const T *() {
        if ( loaded == 0 ) {
            loaded += ( data->setUse ( false ) ? 1 : 0 );
        }

        return data->getConstLocPtr();
    }
    ~adhereToConst() {
        if ( loaded > 0 ) {
            loaded = ( data->unsetUse ( loaded ) ? 0 : loaded );
        }
    }
private:
    const managedPtr<T> *data;
    unsigned int loaded = 0;


    // Test classes
    friend class adhereTo_Unit_LoadUnload_Test;
    friend class adhereTo_Unit_LoadUnloadConst_Test;
    friend class adhereTo_Unit_TwiceAdhered_Test;
};

}

#endif

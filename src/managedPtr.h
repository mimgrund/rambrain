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


//Convenience macros
#define ADHERETO(class,instance) adhereTo<class> instance##_glue(instance);\
                 class* instance = instance##_glue;
#define ADHERETOCONST(class,instance) const adhereTo<class> instance##_glue(instance);\
                 const class* instance = instance##_glue;
#define ADHERETOLOC(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
                 class* locinstance = instance##_glue;
#define ADHERETOLOCCONST(class,instance,locinstance) const adhereTo<class> instance##_glue(instance);\
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
    managedPtr ( const managedPtr<T> &ref ) : chunk ( ref.chunk ), tracker ( ref.tracker ), n_elem ( ref.n_elem ) {
        membrain_atomic_add_fetch ( tracker, 1 );
    }


    managedPtr() : managedPtr ( 1 ) {}

    template <typename... ctor_args>
    managedPtr ( unsigned int n_elem , ctor_args... Args ) {
        this->n_elem = n_elem;
        tracker = new unsigned int;
        ( *tracker ) = 1;
        if ( n_elem == 0 ) {
            return;
        }


#ifdef PARENTAL_CONTROL
        /**I admit, this is a bit complicated. as we do not want users to carry around information about object parenthood
        into their classes, we have to ensure that our 'memoryManager::parent' attribute is carried down the hierarchy correctly.
        As this code part is accessed recursively throughout creation, we have to make sure that
        * we do not lock twice (deadlock)
        * we only continue if we're called by the currently active creation process
        the following is not very elegant, but works. The idea behind this is that we admit one thread at a time to create
        an object class hierarchy. In this way, the parent argument from below is correct.
        This is synced by basically checking wether we should be a master or not based on a possibly setted threadID...
        **/

        bool iamSyncer;
        if ( pthread_mutex_trylock ( &managedMemory::parentalMutex ) == 0 ) {
            //Could lock
            iamSyncer = true;
        } else {
            //Could not lock. Perhaps, I already have a lock:
            if ( pthread_equal ( pthread_self(), managedMemory::creatingThread ) ) {
                iamSyncer = false;//I was called by my parents!
            } else {
                //I need to gain the lock:
                pthread_mutex_lock ( &managedMemory::parentalMutex );
                iamSyncer = true;
            }
        }
        //iamSyncer tells me whether I am the parent thread (not object!)

        //Set our thread id as the one without locking:
        if ( iamSyncer ) {
            managedMemory::creatingThread = pthread_self();

        }
#endif

        chunk = managedMemory::defaultManager->mmalloc ( sizeof ( T ) * n_elem );

#ifdef PARENTAL_CONTROL
        //Now call constructor and save possible children's sake:
        memoryID savedParent = managedMemory::parent;
        managedMemory::parent = chunk->id;
#endif

        setUse();
        for ( unsigned int n = 0; n < n_elem; n++ ) {
            new ( ( ( T * ) chunk->locPtr ) + n ) T ( Args... );
        }
        unsetUse();
#ifdef PARENTAL_CONTROL
        managedMemory::parent = savedParent;
        if ( iamSyncer ) {
            managedMemory::creatingThread = 0;
            //Let others do the job:
            pthread_mutex_unlock ( &managedMemory::parentalMutex );
        }
#endif
    }


    ~managedPtr() {
        int trackerold = membrain_atomic_sub_fetch ( tracker, 1 );
        if ( trackerold  == 0 ) {//Ensure destructor to be called only once.
            mDelete<T>();
        }
    }

    bool prepareUse() const {
        managedMemory::defaultManager->prepareUse ( *chunk, true );
        return true;
    }

    //Atomically sets use if tracker is not already set to true. returns whether we set use or not.
    bool setUse ( bool writable = true, bool *tracker = NULL ) const {
        pthread_mutex_lock ( &mutex );
        if ( tracker )
            if ( !membrain_atomic_bool_compare_and_swap ( tracker, false, true ) ) {
                pthread_mutex_unlock ( &mutex );
                return false;
            }
        bool result = managedMemory::defaultManager->setUse ( *chunk , writable );
        pthread_mutex_unlock ( &mutex );
        return result;
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
        ADHERETOLOCCONST ( T, self, loc );
        return loc[i];
    }

    T *getLocPtr() const {
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
    mutable pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    template <class G>
    typename std::enable_if<std::is_class<G>::value>::type
    mDelete (  ) const {
        if ( n_elem > 0 ) {

            setUse();
            for ( unsigned int n = 0; n < n_elem; n++ ) {
                ( ( ( G * ) chunk->locPtr ) + n )->~G();
            }
            unsetUse();
            managedMemory::defaultManager->mfree ( chunk->id );
        }
        delete tracker;
    }
    template <class G>
    typename std::enable_if < !std::is_class<G>::value >::type
    mDelete (  ) {
        if ( n_elem > 0 ) {
            managedMemory::defaultManager->mfree ( chunk->id );
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
    adhereTo ( const adhereTo<T> &ref ) : data ( ref.data ) {
        loadedReadable = ref.loadedReadable;
        loadedWritable = ref.loadedWritable;
        if ( loadedWritable ) {
            data->setUse ( loadedWritable );
        }
        if ( loadedReadable ) {
            data->setUse ( loadedReadable );
        }

    };

    adhereTo ( const managedPtr<T> &data, bool loadImidiately = false ) : data ( &data ) {
        if ( loadImidiately ) {
            loadedReadable = true;
            data.prepareUse();
        }

    }

    adhereTo<T> &operator= ( const adhereTo<T> &ref ) {
        if ( loadedReadable ) {
            data->unsetUse();
        }
        if ( loadedWritable ) {
            data->unsetUse();
        }
        this->data = ref.data;
        loadedReadable = ref.loadedReadble;
        loadedWritable = ref.loadedWritable;
        if ( loadedWritable ) {
            data->setUse ( loadedWritable );
        }
        if ( loadedReadable ) {
            data->setUse ( loadedReadable );
        }
        return *this;
    }

    operator const T *() { //This one is needed as c++ refuses to pick operator const T *() const as a default in this case
        return * ( ( const adhereTo * ) this );
    }
    operator const T *() const {
        if ( !loadedReadable ) {
            data->setUse ( false, &loadedReadable );
        }
        return data->getConstLocPtr();
    }
    operator  T *() {
        if ( !loadedWritable ) {
            data->setUse ( true, &loadedWritable );
        }
        return data->getLocPtr();
    }

    ~adhereTo() {
        if ( loadedReadable ) {
            data->unsetUse();
        }
        if ( loadedWritable ) {
            data->unsetUse();
        }
    }
private:
    const managedPtr<T> *data;

    mutable bool loadedWritable = false;
    mutable bool loadedReadable = false;

    // Test classes
    friend class ::adhereTo_Unit_LoadUnload_Test;
    friend class ::adhereTo_Unit_LoadUnloadConst_Test;
    friend class ::adhereTo_Unit_TwiceAdhered_Test;
};


}

#endif

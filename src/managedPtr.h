/*   rambrain - a dynamical physical memory extender
 *   Copyright (C) 2015 M. Imgrund, A. Arth
 *   mimgrund (at) mpifr-bonn.mpg.de
 *   arth (at) usm.uni-muenchen.de
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MANAGEDPTR_H
#define MANAGEDPTR_H

#include "managedMemory.h"
#include "rambrain_atomics.h"
#include "common.h"
#include "exceptions.h"
#include <type_traits>
#include <pthread.h>

//Test classes
#ifdef BUILD_TESTS
class managedPtr_Unit_ChunkInUse_Test;
class managedPtr_Unit_GetLocPointer_Test;
class managedPtr_Unit_SmartPointery_Test;
class managedFileSwap_Unit_SwapSingleIsland_Test;
class managedFileSwap_Unit_SwapNextAndSingleIsland_Test;

class adhereTo_Unit_LoadUnload_Test;
class adhereTo_Unit_LoadUnloadConst_Test;
class adhereTo_Unit_TwiceAdhered_Test;
class adhereTo_Unit_TwiceAdheredOnceUsed_Test;
#endif

namespace rambrain
{

template <class T>
class adhereTo;


//Convenience macros
///@brief adheres to a class member 'instance' shadowing it and making available the data under *instance
#define ADHERETO(class,instance) adhereTo<class> instance##_glue(instance);\
                 class* instance = instance##_glue;
///const version of @see ADHERETO
#define ADHERETOCONST(class,instance) const adhereTo<class> instance##_glue(instance);\
                 const class* instance = instance##_glue;
///@brief adheres to instance of type managedPtr< class > and name data pointer locinstance
#define ADHERETOLOC(class,instance,locinstance) adhereTo<class> instance##_glue(instance);\
                 class* locinstance = instance##_glue;
///const version of @see ADHERETOLOC
#define ADHERETOLOCCONST(class,instance,locinstance) const adhereTo<class> instance##_glue(instance);\
                 const class* locinstance = instance##_glue;



/**
 * \brief Main class to allocate memory that is managed by the rambrain memory defaultManager
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
    ///@brief copy ctor
    managedPtr ( const managedPtr<T> &ref ) : chunk ( ref.chunk ), tracker ( ref.tracker ), n_elem ( ref.n_elem ) {
        rambrain_atomic_add_fetch ( tracker, 1 );
    }


    ///@brief with no arguments given, instantiates an array with one element
    managedPtr() : managedPtr ( 1 ) {}

    ///@brief instantiates managedPtr containing n_elem elements and passes Args as arguments to the constructor of these
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
                rambrain_pthread_mutex_lock ( &managedMemory::parentalMutex );
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
            rambrain_pthread_mutex_unlock ( &managedMemory::parentalMutex );
        }
#endif
    }

    ///@brief destructor
    ~managedPtr() {
        int trackerold = rambrain_atomic_sub_fetch ( tracker, 1 );
        if ( trackerold  == 0 ) {//Ensure destructor to be called only once.
            mDelete<T>();
        }
    }

    /// @brief Simple getter
    inline  unsigned int size() const {
        return n_elem;
    }

    ///@brief tells the memory manager to possibly swap in chunk for near future use
    bool prepareUse() const {
        managedMemory::defaultManager->prepareUse ( *chunk, true );
        return true;
    }

    ///@brief Atomically sets use to a chunk if tracker is not already set to true. returns whether we set use or not.
    bool setUse ( bool writable = true, bool *tracker = NULL ) const {
        if ( tracker )
            if ( !rambrain_atomic_bool_compare_and_swap ( tracker, false, true ) ) {
                waitForSwapin();
                return false;
            }
        bool result = managedMemory::defaultManager->setUse ( *chunk , writable );

        return result;
    }

    ///@brief unsets use count on memory chunk
    bool unsetUse ( unsigned int loaded = 1 ) const {
        return managedMemory::defaultManager->unsetUse ( *chunk , loaded );
    }

    ///@brief assignment operator
    managedPtr<T> &operator= ( const managedPtr<T> &ref ) {
        if ( ref.chunk == chunk ) {
            return *this;
        }
        if ( rambrain_atomic_sub_fetch ( tracker, 1 ) == 0 ) {
            mDelete<T>();
        }
        n_elem = ref.n_elem;
        chunk = ref.chunk;
        tracker = ref.tracker;
        rambrain_atomic_add_fetch ( tracker, 1 );
        return *this;
    }

    ///@note This operator can only be called safely in single threaded situations when preemptive swapOut actions have been disabled
    DEPRECATED T &operator[] ( int i ) {
        managedPtr<T> &self = *this;
        ADHERETOLOC ( T, self, loc );
        return loc[i];
    }
    ///@note This operator can only be called safely in single threaded situations when preemptive swapOut actions have been disabled
    DEPRECATED const T &operator[] ( int i ) const {
        const managedPtr<T> &self = *this;
        ADHERETOLOCCONST ( T, self, loc );
        return loc[i];
    }

    /** @brief returns local pointer to object
     *  @note when called, you have to care for setting use to the chunk, first
     * */
    T *getLocPtr() const {
        if ( chunk->status != MEM_ALLOCATED_INUSE_WRITE ) {
            waitForSwapin();
        }
        return ( T * ) chunk->locPtr;
    }

    /** @brief returns const local pointer to object
     *  @note when called, you have to care for setting use to the chunk, first
     * */
    const T *getConstLocPtr() const {
        if ( ! ( chunk->status & MEM_ALLOCATED_INUSE_READ ) ) {
            waitForSwapin();
        }
        return ( T * ) chunk->locPtr;
    }

private:
    managedMemoryChunk *chunk;
    unsigned int *tracker;
    unsigned int n_elem;

    ///@brief This function manages correct deallocation for array elements having a destructor
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
    ///@brief This function manages correct deallocation for array elements lacking a destructor
    template <class G>
    typename std::enable_if < !std::is_class<G>::value >::type
    mDelete (  ) {
        if ( n_elem > 0 ) {
            managedMemory::defaultManager->mfree ( chunk->id );
        }
        delete tracker;
    }

    /** @brief: indefinitely waits for swapin of the chunk
     *  While it would have been desirable to throw exceptions when chunk is not available,
     *  checking this in a save way would produce lots of overhead. Thus, it is better to wait
     *  indefinitely in this case, as under normal use, the chunk will become available.
    **/
    void waitForSwapin() const {
        //While in this case, we are not the guy who actually enforce the swapin, we never the less have to wait
        //for the chunk to become ready. This is done in the following way:
        if ( ! ( chunk->status & MEM_ALLOCATED ) ) { // We may savely check against this as use will be set by other adhereTo thread and cannot be undone as long as calling adhereTo exists
            rambrain_pthread_mutex_lock ( &managedMemory::defaultManager->stateChangeMutex );
            //We will burn a little bit of power here, eventually, but this is a very rare case.
            while ( ! managedMemory::defaultManager->waitForSwapin ( *chunk, true ) ) {};
            rambrain_pthread_mutex_unlock ( &managedMemory::defaultManager->stateChangeMutex );
        }
    }




    template<class G>
    friend class adhereTo;
    template<class G>
    friend class adhereToConst;

    //Test classes
#ifdef BUILD_TESTS
    friend class ::managedPtr_Unit_ChunkInUse_Test;
    friend class ::managedPtr_Unit_GetLocPointer_Test;
    friend class ::managedPtr_Unit_SmartPointery_Test;
    friend class ::managedFileSwap_Unit_SwapSingleIsland_Test;
    friend class ::managedFileSwap_Unit_SwapNextAndSingleIsland_Test;
    friend class ::adhereTo_Unit_TwiceAdheredOnceUsed_Test;
#endif
};

/**
 * @brief Main class to fetch memory that is managed by rambrain for actual usage.
 *
 * \warning _thread-safety_
 * * The object itself is not thread-safe
 * * Do not pass pointers/references to this object over thread boundaries
 *
 * \note _thread-safety_
 * * The object itself may be copied over thread boundaries
 * **/
template <class T>
class adhereTo
{
public:
    ///@brief copy constructor
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

    /**@brief constructor fetching data
     * \param loadImmediately set this to false if you want to load the element when pulling the pointer and not beforehands
     * \param data the managedPtr that is to be used in near future
    **/
    adhereTo ( const managedPtr<T> &data, bool loadImmediately = true ) : data ( &data ) {
        if ( loadImmediately && data.size() != 0 ) {
            data.prepareUse();
        }

    }

    /// Provides the same functionality as the other constructor but accepts a managedPtr pointer as argument. @see adhereTo ( const managedPtr<T> &data, bool loadImmediately = true )
    adhereTo ( const managedPtr<T> *data, bool loadImmediately = true ) : adhereTo ( *data, loadImmediately ) {};

    ///Simple assignment operator
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

    ///@brief This operator can be used to pull the data to a const pointer. Use this whenever possible.
    operator const T *() { //This one is needed as c++ refuses to pick operator const T *() const as a default in this case
        return * ( ( const adhereTo * ) this );
    }
    ///@brief This operator can be used to pull the data to a const pointer. Use this whenever possible.
    operator const T *() const {
        if ( data->size() == 0 ) {
            return NULL;
        }
        if ( !loadedReadable ) {
            data->setUse ( false, &loadedReadable );
        }
        return data->getConstLocPtr();
    }
    ///@brief This operator can be used to pull the data to a non-const pointer. If you only read the data, pull the const version, as this saves execution time.
    operator  T *() {
        if ( data->size() == 0 ) {
            return NULL;
        }
        if ( !loadedWritable ) {
            data->setUse ( true, &loadedWritable );
        }
        return data->getLocPtr();
    }
    ///@brief destructor
    ~adhereTo() {
        unsigned char loaded = 0;
        loaded = ( loadedReadable ? 1 : 0 ) + ( loadedWritable ? 1 : 0 );
        if ( loaded > 0 && data->size() != 0 ) {
            data->unsetUse ( loaded );
        }
    }
private:
    const managedPtr<T> *data;

    mutable bool loadedWritable = false;
    mutable bool loadedReadable = false;

    //Test classes
#ifdef BUILD_TESTS
    friend class ::adhereTo_Unit_LoadUnload_Test;
    friend class ::adhereTo_Unit_LoadUnloadConst_Test;
    friend class ::adhereTo_Unit_TwiceAdhered_Test;
    friend class ::adhereTo_Unit_TwiceAdheredOnceUsed_Test;
#endif
};

/** @brief this class marks a section as globally critical. Only one thread can process any section where such an object is generated.
 *
 * if called with locksByUser set to true, the user has to unlock the mutex manually calling unlock.
**/

class rambrainGlobalCriticalSectionControl
{
public:
    rambrainGlobalCriticalSectionControl ( bool locksByUser = false ) : locksByUser ( locksByUser ) {
        if ( !locksByUser ) {
            start();
        }
    };
    ~rambrainGlobalCriticalSectionControl() {
        if ( !locksByUser ) {
            stop();
        }
    }
    /// stop pulling critical ingredients in a multithreaded situation
    void stop() {
        rambrain_pthread_mutex_unlock ( &mutex );
    }
    /// start pulling of critical ingredients in a multithreaded situation
    void start() {
        rambrain_pthread_mutex_lock ( &mutex );
    }
private:
    static pthread_mutex_t mutex; // is defined in managedMemory.cpp
    bool locksByUser;

};

/// @brief put this macro in a scope and define what data your thread needs multithreading-safe
#define LISTOFINGREDIENTS rambrainGlobalCriticalSectionControl rambrain_section_control;


}

#endif


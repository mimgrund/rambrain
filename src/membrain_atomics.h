#ifndef MEMBRAINATOMICS_H
#define MEMBRAINATOMICS_H

//We want to use a clever logic here to be more portable...

//GCC atomics
#define membrain_atomic_fetch_add(a,b) __sync_fetch_and_add(a,b)
#define membrain_atomic_fetch_sub(a,b) __sync_fetch_and_sub(a,b)
#define membrain_atomic_add_fetch(a,b) __sync_add_and_fetch(a,b)
#define membrain_atomic_sub_fetch(a,b) __sync_sub_and_fetch(a,b)
#define membrain_atomic_bool_compare_and_swap(ptr,oldval,newval) __sync_bool_compare_and_swap(ptr,oldval,newval)


#endif
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

#ifndef COMMON_H
#define COMMON_H

#include <malloc.h>
#include <inttypes.h>

namespace rambrain
{

typedef double myScalar;

inline static double sqr ( double x )
{
    return x * x;
}
#define NANCHECK(x) if(x!=x) errmsg("NaN! occured")
#define NANBOOL(x) x!=x?true:false

#define errmsg(message) {fprintf(stderr,"\033[31mERROR:\033[0m\t%s\n\t\033[90m(from %s, %s(...) , line %d)\033[0m\n", message, __FILE__,__FUNCTION__,__LINE__);}
#define infomsg(message) {fprintf(stderr,"\033[32mINFO:\033[0m\t%s\n\t\033[90m(from %s, %s(...) , line %d)\033[0m\n", message, __FILE__,__FUNCTION__,__LINE__);}
#define warnmsg(message) {fprintf(stderr,"\033[33mWARN:\033[0m\t%s\n\t\033[90m(from %s, %s(...) , line %d)\033[0m\n", message, __FILE__,__FUNCTION__,__LINE__);}

#define errmsgf(format,...) {char tmp[1024];snprintf(tmp,1024,format,__VA_ARGS__);errmsg(tmp);}
#define infomsgf(format,...) {char tmp[1024];snprintf(tmp,1024,format,__VA_ARGS__);infomsg(tmp);}
#define warnmsgf(format,...) {char tmp[1024];snprintf(tmp,1024,format,__VA_ARGS__);warnmsg(tmp);}

#ifdef DBG_MUTICES
#define rambrain_pthread_mutex_lock(x) infomsg("Lock of " #x " ") pthread_mutex_lock(x);
#define rambrain_pthread_mutex_unlock(x) infomsg("Unlock of " #x " ") pthread_mutex_unlock(x);
#else
#define rambrain_pthread_mutex_lock(x) pthread_mutex_lock(x)
#define rambrain_pthread_mutex_unlock(x) pthread_mutex_unlock(x)
#endif
#define VECTOR_FOREACH(vec,iter) for(int iter = 0; iter < vec.size(); ++iter)

#ifdef __GNUC__
#define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED
#endif

typedef uint64_t global_bytesize;

const global_bytesize kib = 1024;
const global_bytesize mib = kib * kib;
const global_bytesize gig = mib * kib;

}

#endif


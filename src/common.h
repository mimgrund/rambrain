#ifndef COMMON_H
#define COMMON_H

#include <malloc.h>


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

#define VECTOR_FOREACH(vec,iter) for(int iter = 0; iter < vec.size(); ++iter)

#endif

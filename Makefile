GIT_VERSION = $(shell sh -c 'git log -1|head -1|cut -d " " -f2')
GIT_DIFF = $(shell sh -c 'git diff>gitdiff.bin')

SOURCES= managedMemory managedPtr
LIBRARIES = m
LIB = libmembrain.so.1.0
LIBPATH = /usr/lib64
INCPATH = /usr/include
CFLAGS = -Wall -fPIC -g -D__GIT_VERSION=\"$(GIT_VERSION)\"
LINKFLAGS = -shared -Wl,-soname,membrain.so.1 -ldl


SRCFILES = $(SOURCES:%=%.cpp)
CMPFILES = $(SOURCES:%=%.o)
LIBLINK = $(LIBRARIES:%=-l%)


COMPILE = g++ -c -std=c++11 
LINK = g++ 

%.o : %.cpp git_info.h
	$(COMPILE) $(CFLAGS) $<

lib : git_info.h $(CMPFILES) $(SRCFILES)
	$(LINK) $(LINKFLAGS) $(CMPFILES) $(LIBLINK) -o $(LIB) 

git_info.h : $(SRCFILES) gengit.sh
	/bin/sh gengit.sh

install : lib
	mkdir -p $(INCPATH)/membrain
	cp *.h $(INCPATH)/membrain
   
	mv $(LIB) $(LIBPATH)
	ln -sf $(LIBPATH)/$(LIB) $(LIBPATH)/libmembrain.so.1
	ln -sf $(LIBPATH)/membrain.so.1 $(LIBPATH)/libmembrain.so

test : git_info.h $(CMPFILES) $(SRCFILES) test.o
	$(LINK) $(CMPFILES) test.o $(LIBLINK) -lgtest  -o membrain_test

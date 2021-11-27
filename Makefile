CC=clang  #If you use GCC, add -fno-strict-aliasing to the CFLAGS because the Google BTree does weird stuff
#CFLAGS=-Wall -O0 -ggdb3
#CFLAGS=-O2 -ggdb3 -Wall 
#CFLAGS=-O2 -ggdb3 -Wall -I/home/tiancheng/ralloc/src -DRALLOC=ralloc
#CFLAGS=-O2 -ggdb3 -Wall  -fno-omit-frame-pointer
CFLAGS=-O0 -ggdb3 -Wall  -fno-omit-frame-pointer

CXX=clang++
CXXFLAGS= ${CFLAGS} -std=c++14

#LDLIBS=-lm -lpthread -lstdc++
LDLIBS=-lm -lpthread -lstdc++ -lvmmalloc -lpmem -lrt -lnuma
#LDLIBS= -lm -lpthread -lstdc++ -lpmem -lrt -lnuma \
-L/home/tiancheng/ralloc/test /home/tiancheng/ralloc/test/libralloc.a

LDFLAGS=-Wl,-rpath,/usr/local/lib

MICROBENCH_OBJ=microbenchPara.o fast_treap.o

.PHONY: all clean

all: makefile.dep microbenchPara

#makefile.dep: *.[Cch]
#	for i in *.[Cc]; do ${CC} -MM "$${i}" ${CFLAGS}; done > $@
makefile.dep: *.cc *.h
	for i in *.cc; do ${CXX} -MM "$${i}" ${CXXFLAGS}; done > $@

-include makefile.dep

microbenchPara: $(MICROBENCH_OBJ)

clean:
	rm -f *.o microbenchPara


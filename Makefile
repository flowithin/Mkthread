UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
    CC=clang++
    CC+=-D_XOPEN_SOURCE -Wno-deprecated-declarations
    LIBCPU=libcpu_macos.o
else
    CC=g++
    LIBCPU=libcpu.o
endif

CC+=-g -Wall -std=c++17 ${CFLAGS}

# List of source files for your thread library
THREAD_SOURCES=cv.cpp mutex.cpp thread.cpp cpu.cpp 

# Generate the names of the thread library's object files
THREAD_OBJS=${THREAD_SOURCES:.cpp=.o}

all: libthread.o 

# Compile the thread library and tag this compilation
libthread.o: ${THREAD_OBJS}
	./autotag.sh 
	ld -r -o $@ ${THREAD_OBJS}

# Compile an application program
test1: test1.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread

test2: test2.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
test3: test3.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
test4: test4.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
test5: test5.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread

test_lock: test_lock.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
test_join: test_join.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
pizza: testpiz.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
pizza2: testpiz2.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
pizza3: testpiz3.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
pizza3y: testpiz3y.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
selfjoin: test_self_join.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread



lexit: testlockexit.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
 
lr: testrel.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
testmanylock: testmanylocks.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread

testdead: testdead.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
sep: testsep.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
ping: testpingpong.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
td: testdunl.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread
cvsl: test_cv_not_own_lock.cpp libthread.o ${LIBCPU}
	${CC} -o $@ $^ -ldl -pthread









# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${THREAD_OBJS} libthread.o test1

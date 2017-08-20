INCLUDE_DIRS =  -I/usr/local/opencv/include
LIB_DIRS =
CC=g++

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS) -std=c++11
LIBS= -lrt
CPPLIBS= -L/usr/local/opencv/lib -lopencv_core -lopencv_flann -lopencv_video -lpthread -lrt

HFILES=
CFILES=

SRCS= ${HFILES} ${CFILES}

#all:	videothread
all:	videothread

clean:
	-rm -f *.o *.d *.ppm *.jpg

	-rm -f videothread


distclean:
	-rm -f *.o *.d

videothread: videothread.o
	$(CC) $(LDFLAGS) $(CFLAGS) $(INCLUDE_DIRS) -o $@ $@.o `pkg-config --libs opencv` $(CPPLIBS)


depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.o:
	$(CC) $(CFLAGS) -c $<

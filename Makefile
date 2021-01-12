CC = gcc

CFLAGS = -Wall -g -fpic

intel-all: lib/libmalloc.so lib64/libmalloc.so
	ar r libmalloc.a malloc32.o
	mv libmalloc.a lib/libmalloc.a
	ar r libmalloc.a malloc64.o
	mv libmalloc.a lib64/libmalloc.amak

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib malloc64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o

lib:
	mkdir lib

lib64:
	mkdir lib64

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

malloc: malloc.o
	$(CC) $(CFLAGS) -o malloc malloc.o

malloc.o: malloc.c definitions.h
	 $(CC) $(CFLAGS) -c malloc.c

clean: malloc malloc.o malloc32.o malloc64.o
	rm malloc
	rm malloc.o
	rm malloc32.o
	rm malloc64.o

run: malloc
	./malloc

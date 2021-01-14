CC = gcc

CFLAGS = -Wall -g -fpic

malloc: intel-all
	@echo done

libmalloc.a:
	ar r lib/libmalloc.a malloc32.o
	ar r lib64/libmalloc.a malloc64.o


libmalloc.so: lib/libmalloc.so lib64/libmalloc.so

lib:
	mkdir lib
lib64:	
	mkdir lib64


intel-all: lib lib64 lib/libmalloc.so lib64/libmalloc.so
	

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib malloc64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

malloc.o: malloc.c definitions.h
	$(CC) $(CFLAGS) -c malloc.c

clean: malloc malloc.o malloc32.o malloc64.o
	rm malloc.o
	rm malloc32.o
	rm malloc64.o

build: lib64/libmalloc.so prog.c 
	$(CC) $(CFLAGS) -o prog prog.c lib64/libmalloc.so -L. 

run: build
	./prog

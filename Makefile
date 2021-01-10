CC = gcc

CFLAGS = -Wall -pedantic -g


malloc: malloc.o
	$(CC) $(CFLAGS) -o malloc malloc.o
malloc.o: malloc.c definitions.h
	 $(CC) $(CFLAGS) -c malloc.c
run: malloc
	./malloc

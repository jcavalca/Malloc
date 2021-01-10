CC = gcc

CFLAGS = -Wall -pedantic -g


malloc: malloc.o
	$(CC) $(CFLAGS) -o malloc malloc.o
malloc.o: malloc.c definitions.h
	 $(CC) $(CFLAGS) -c malloc.c
clean: malloc malloc.o
	rm malloc
	rm malloc.o
run: malloc
	./malloc

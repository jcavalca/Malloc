#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<unistd.h>

#include"header.h"

#define HUNK_SIZE 65536
#define TRUE 1
#define FALSE 0

/* Global variables for storing heap info */

int firstTime = TRUE; /*first time calling sbrk?*/
size_t headerSize = sizeof(Header);
uintptr_t *startHeap;
Header *firstHeader;
Header *lastHeader;
uint64_t heapSize = 0;

/*This rounds a number to the next 16 multiple*/
size_t round_16(size_t number){
    size_t multiple = 16;
    while (multiple < number){
        multiple = multiple * 16;
    }
    return multiple;
}

/*Initializes a 64k heap header*/
Header *initializeHunk(void *start){
    Header startHeader;
    startHeader.size = HUNK_SIZE - round_16(headerSize); /*size to store data*/
    startHeader.free = TRUE;
    startHeader.nextHeader = NULL; /*starts with one header*/
    * ((Header *)start) = startHeader;
    return start;
}

/*Returns a header with size that fits what was asked, 
    otherwise return NULL and sets lastHeader global 
    variable. */
Header *findBigEnoughBlock(size_t desired_size){
    Header *currentHeader = firstHeader;
    int sizeOfHeader = round_16(headerSize);
    while (currentHeader != NULL){
        if (currentHeader -> free != FALSE){
            int sizeAvailable = currentHeader -> size;

            /*Matches size perfectly*/
            if (sizeAvailable == desired_size){
                return currentHeader;
            }
            /*Bigger so split big block into 2 blocks*/
            if ( sizeAvailable > (desired_size + sizeOfHeader) ){
                char *address = (char *) currentHeader + desired_size;

   		        Header newHeader;
                newHeader.size =  sizeAvailable - desired_size - sizeOfHeader; /*size to store data*/
                newHeader.free = TRUE;
                newHeader.nextHeader = currentHeader -> nextHeader; /*starts with one header*/
    
		        * ((Header*) address) = newHeader;
        	    currentHeader -> nextHeader = address;

		        currentHeader -> size = desired_size;
                return currentHeader;
            }
        }
        if (currentHeader -> nextHeader == NULL){
            lastHeader = currentHeader;
        }
        currentHeader = currentHeader -> nextHeader;
    }
    return NULL;
}

/* This function starts at the first heap header and joins 2 adjacent free blocks as it traverses the heap.*/
void joinAdjacentFreeBlocks(){
    Header *old = firstHeader;
    Header *next = firstHeader -> nextHeader;

    while (next != NULL){
        /*If both are free, join them */
        if (  (old -> free == TRUE)  &&  (next -> free == TRUE) ){

            /* new size = old + new - header */
            int old_size = old -> size;
            int next_size = next -> size;
            int totalSize = old_size + next_size + round_16(headerSize);
            old -> size = totalSize;

            old -> nextHeader = next -> nextHeader;
            next = old -> nextHeader;
        }
        /*If they aren't both free, keep searching*/
        else{
            old = next;
            next = next -> nextHeader;
        }
    }

}


void *my_malloc(size_t desired_size){
    Header *return_header;
    if (firstTime == TRUE){
        if (  (startHeap = sbrk(HUNK_SIZE) ) == (void *) -1 ){
            perror("fail sbrk");
            return NULL;
        }
	heapSize = heapSize + HUNK_SIZE;
        firstHeader = initializeHunk(startHeap);
        lastHeader = firstHeader;
        firstTime = FALSE;
    }
    while((return_header = findBigEnoughBlock(desired_size) ) == NULL){
        /*Case 1: Last header in heap is free and we join with a new sbrk call.*/
        if (lastHeader -> free == TRUE){
            if (sbrk(HUNK_SIZE) == (void *) -1){
                perror("fail sbrk");
                return NULL;
            }
	    heapSize = heapSize + HUNK_SIZE;
            lastHeader -> size = lastHeader -> size + HUNK_SIZE;
        }
        /*Case 2: Last header is full so we have to initialize the new 64k block with a free header*/
        else{
            if ( (lastHeader = sbrk(HUNK_SIZE) ) == (void *) -1){
                perror("fail sbrk");
                return NULL;
            }
       	    heapSize = heapSize + HUNK_SIZE;
	    initializeHunk(lastHeader);
        }
        
    }

    return_header -> free = FALSE;

    return (char*)return_header + round_16(headerSize);
}

int main(int agrc, char* argv[]){

    int *pointer = my_malloc(sizeof(int));
    int *pointer2 = my_malloc(sizeof(int));
    char *subject = my_malloc(30);
    char *bigName;

    subject = "joao cavalcanti";
    pointer[0] = 0;
    pointer2[0] = 1;
    bigName = my_malloc(HUNK_SIZE);
    bigName = "testing beyond boundary";

   /* printf("pointer has %d\n", pointer[0]);
    printf("pointer has %d\n", pointer2[0]);
    printf("subject %s\n", subject);*/
    printf("big word: %s\n", bigName);
    exit(EXIT_SUCCESS);

}

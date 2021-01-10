#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<unistd.h>

#include"definitions.h"

#define HUNK_SIZE 65536
#define TRUE 1
#define FALSE 0

/* Global variables for storing heap info */

int firstTime = TRUE; /*first time calling sbrk?*/
size_t headerSize = sizeof(Header);
intptr_t startHeap;
Header *firstHeader;
Header *lastHeader;
uint64_t heapSize = 0;

/*This rounds a number to the next 16 multiple*/
size_t round_16(size_t number){
    size_t multiple = 16;
    int count = 2;
    while (multiple < number){
        multiple = multiple * count;
        count++;
    }
    return multiple;
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

/*Initializes a 64k heap header*/
intptr_t initializeHunk(intptr_t start){
    size_t adjustedSize = HUNK_SIZE - round_16(headerSize);
    Header *startHeader;
    startHeader = (Header *)start;
    startHeader -> size = adjustedSize;  /*size to store data*/
    startHeader -> free = TRUE;
    startHeader -> nextHeader = NULL; /*starts with one header*/
    return start;
}

/*Returns a header with size that fits what was asked, 
    otherwise return NULL and sets lastHeader global 
    variable. */
intptr_t findBigEnoughBlock(size_t desired_size){
    Header *currentHeader = firstHeader;
    size_t sizeOfHeader = round_16(headerSize);
    while (currentHeader != NULL){
        if (currentHeader -> free != FALSE){
            size_t sizeAvailable = currentHeader -> size;

            /*Matches size perfectly*/
            if (sizeAvailable == desired_size){
                return (intptr_t) currentHeader + sizeOfHeader;
            }
            /*Bigger so split big block into 2 blocks*/
            if ( sizeAvailable > (desired_size + sizeOfHeader) ){
                intptr_t address = (intptr_t) currentHeader + sizeOfHeader + desired_size;

   		        Header *newHeader = (Header *) address;
                newHeader -> size =  sizeAvailable - desired_size - sizeOfHeader; /*size to store data*/
                newHeader -> free = TRUE;
                newHeader -> nextHeader = currentHeader -> nextHeader; /*starts with one header*/
    
        	    currentHeader -> nextHeader = newHeader;

		        currentHeader -> size = desired_size;
                currentHeader -> free = FALSE;
                return address + sizeOfHeader;
            }
        }
        if (currentHeader -> nextHeader == NULL){
            lastHeader = currentHeader;
        }
        currentHeader = currentHeader -> nextHeader;
    }
    return (intptr_t) NULL;
}

/*Calss malloc and then zeroes out everything*/
void *my_calloc(size_t desired_size){
    void *ret_address = my_malloc(desired_size);
    uint8_t *zero_counter = ret_address;
    int count;
    for (count = 0; count < desired_size; count++){
        *(zero_counter + count) = 0;
    }
    return ret_address;
}

void *my_malloc(size_t desired_size){
    void *return_address;
    if (firstTime == TRUE){
        if (  (void *)(startHeap = (intptr_t) sbrk(HUNK_SIZE) ) == (void *) -1 ){
            perror("fail sbrk");
            return NULL;
        }
	    heapSize = heapSize + HUNK_SIZE;
        firstHeader = (Header *) initializeHunk(startHeap);
        lastHeader = firstHeader;
        firstTime = FALSE;
    }
    while((return_address = (void*) findBigEnoughBlock(desired_size) ) == NULL){
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
	    initializeHunk((intptr_t)lastHeader);
        }
        
    }

    return   return_address;
}

int main(int agrc, char* argv[]){

    int *pointer = my_malloc(sizeof(int));
    int *pointer2 = my_calloc(sizeof(int));
    char *subject = my_malloc(30);
    char *bigName;

    subject = "joao cavalcanti";
    pointer[0] = 0;
    pointer2[0] = 1;
    bigName = my_malloc(HUNK_SIZE);
    bigName = "testing beyond boundary";

    printf("pointer has %d\n", pointer[0]);
    printf("pointer has %d\n", pointer2[0]);
    printf("subject %s\n", subject);
    printf("big word: %s\n", bigName);
    exit(EXIT_SUCCESS);

}

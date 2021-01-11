#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>

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
                currentHeader -> free = FALSE;
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
                return (intptr_t)currentHeader + sizeOfHeader;
            }
        }
        if (currentHeader -> nextHeader == NULL){
            lastHeader = currentHeader;
        }
        currentHeader = currentHeader -> nextHeader;
    }
    return (intptr_t) NULL;
}

void my_free(void *ptr){
    size_t sizeOfHeader = round_16(headerSize);
    intptr_t input = (intptr_t) ptr;
    Header *currentHeader;
    Header *prevHeader;
    /*Bad users ...*/
    if (ptr == NULL){
        return;
    }

    /*Out of heap ptr*/
    if (  (input < (intptr_t) startHeap) || 
          (input > (intptr_t) startHeap + heapSize)){
        return;
    }
    /*On Heap*/
    currentHeader = firstHeader;
    prevHeader = firstHeader;

    while(  input > (intptr_t) (currentHeader ->nextHeader) + sizeOfHeader){
        prevHeader = currentHeader;
        currentHeader = currentHeader -> nextHeader;
    }
    if (currentHeader != NULL){
        currentHeader -> free = TRUE;

        /*Case 1: Both previous and next blocks are free*/
        if(   (prevHeader != currentHeader && prevHeader -> free == TRUE) &&
              (currentHeader -> nextHeader != NULL) && 
              (currentHeader -> nextHeader -> free == TRUE)){
            size_t totalSize = prevHeader -> size + currentHeader -> size + 
                                currentHeader -> nextHeader -> size + 2*sizeOfHeader;
            Header *newNext = currentHeader -> nextHeader -> nextHeader;
            prevHeader -> size = totalSize;
            prevHeader -> nextHeader = newNext; 
        }
        /*Case 2: Only previous is free*/
        else if(prevHeader != currentHeader && prevHeader -> free == TRUE){
            size_t totalSize = prevHeader -> size + currentHeader -> size + sizeOfHeader;
            Header *newNext = currentHeader -> nextHeader;
            prevHeader -> size = totalSize;
            prevHeader -> nextHeader = newNext; 
        }

        /*Case 3: Only next is free*/
        else if ((currentHeader -> nextHeader != NULL) && 
              (currentHeader -> nextHeader -> free == TRUE)){
            size_t totalSize = currentHeader -> size + currentHeader -> nextHeader -> size + sizeOfHeader;
            Header *newNext = currentHeader -> nextHeader -> nextHeader;
            currentHeader -> size = totalSize;
            currentHeader -> nextHeader = newNext;
            currentHeader -> free = TRUE;

        }
    }
}


/*Calls malloc and then zeroes out everything*/
void *my_calloc(size_t desired_size){
    int count;
    void *ret_address;
    uint8_t *zero_counter;

    /*Bad users ...*/
    if (desired_size == 0){
        return NULL;
    }

    ret_address = my_malloc(desired_size);
    /*error checking*/
    if (ret_address == NULL)
        return NULL;

    zero_counter = ret_address;

    for (count = 0; count < desired_size; count++){
        *(zero_counter + count) = 0;
    }
    return ret_address;
}

/**/
void *my_realloc(void *ptr, size_t size){
    Header *ptrHeader = firstHeader;
    intptr_t input = (intptr_t) ptr;
    size_t sizeOfHeader = round_16(headerSize);
    size_t originalSize;

    if (ptr == NULL){
        return my_malloc(size);
    }
    if (size == 0 && ptr != NULL){
        my_free(ptr);
        return NULL;
    }
    else if(size == 0){
        return NULL;
    }


    /*Out of heap ptr*/
    if (  (input < (intptr_t) startHeap) || 
          (input > (intptr_t) startHeap + heapSize)){
        return NULL;
    }

    while(  input > (intptr_t) (ptrHeader ->nextHeader) + sizeOfHeader){
        ptrHeader = ptrHeader -> nextHeader;
    }
    if (ptrHeader == NULL){
        return NULL;
    }
    originalSize = ptrHeader -> size;
    /*Case 1: original size =  size*/
    if (originalSize == size){
        return ptr;
    }
 
    /*Case 2: original size > size */
    if ( originalSize > size){
        /*Case 2.1: there is space for a new header*/
        if (originalSize > size + sizeOfHeader){
            

            intptr_t address = (intptr_t) ptrHeader + sizeOfHeader + size;

            Header *newHeader = (Header *) address;
            ptrHeader -> size = size;
            newHeader -> size =  originalSize - size - sizeOfHeader; 
            newHeader -> free = TRUE;
            newHeader -> nextHeader = ptrHeader -> nextHeader; 

            ptrHeader -> nextHeader = newHeader;
            ptrHeader -> size = size;
            ptrHeader -> free = FALSE;

            return (void *) ptr;
        }
        /*Case 2.2: no space for new header*/
        else{
            uint8_t *newPlace = (uint8_t *) my_malloc(size);
            uint8_t *copyVar =  (uint8_t *) ( (intptr_t) ptrHeader + sizeOfHeader );
            size_t count;
            
            /*error checking*/
            if (newPlace == NULL)
                return NULL;

            for (count = 0; count < size; count++){
                *(newPlace + count) = *(copyVar + count);
            }
            ptrHeader -> free = TRUE;
            return newPlace;
        }
    }

    /*Case 3: original size < size*/
    if (originalSize < size){
        /*Case 3.1: Can use next header*/
        if ( (ptrHeader -> nextHeader != NULL) && (ptrHeader -> nextHeader -> free == TRUE) &&
             (ptrHeader -> size + ptrHeader -> nextHeader -> size > size + sizeOfHeader)){

            intptr_t address = (intptr_t) ptrHeader + sizeOfHeader + size;
            size_t totalSize = originalSize + ptrHeader -> nextHeader -> size;

            Header *newHeader = (Header *) address;
            newHeader -> size =  totalSize - size - sizeOfHeader; 
            newHeader -> free = TRUE;
            newHeader -> nextHeader = ptrHeader -> nextHeader -> nextHeader; 

            ptrHeader -> size = size;
            ptrHeader -> nextHeader = newHeader;
            return ptr;
        }
        /*Case 3.2: Have to move location*/
        else{
            uint8_t *newPlace = (uint8_t *) my_malloc(size);
            uint8_t *copyVar =  (uint8_t *) ((intptr_t) ptrHeader + sizeOfHeader);
            int count;

            /*error checking*/
            if (newPlace == NULL)
                return NULL;


            for (count = 0; count < size; count++){
                *(newPlace + count) = *(copyVar + count);
            }
            ptrHeader -> free = TRUE;
            return newPlace;
        }
    }
    return NULL;
}


void *my_malloc(size_t desired_size){
    void *return_address;

     /*Bad users ...*/
    if (desired_size == 0){
        return NULL;
    }

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

    return return_address;
}

int main(int agrc, char* argv[]){
   
    int *pointer = my_malloc(5 * sizeof(int));
    int count;

    my_free(pointer);
    for (count = 0; count < 10000000; count++){
        pointer = my_malloc(5 * sizeof(int));
        pointer = my_realloc(pointer, 5 * sizeof(int));
        my_free(pointer);

    }
    pointer = my_malloc(5 * sizeof(int));


    pointer[0] = 0;
    pointer[1] = 1;
    pointer[2] = 2;

    
   
    if (pointer == NULL){
        printf("NULL\n");
        return 0;
    }
    for (count = 3; count < 15; count++){
        pointer[count] = count;
    }
    for (count = 0; count < 15; count++){
        printf("pointer[%d]: %d\n", count, pointer[count]);
    }

    exit(EXIT_SUCCESS);

}


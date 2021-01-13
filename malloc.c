#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>
#include<limits.h>

#include"definitions.h"

#define HUNK_SIZE 65536
#define TRUE 1
#define FALSE 0
#define LARGE_BUFFER 256

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

/*Prints status if DEBUG_MALLOC env variable is set*/
void print_status_malloc(size_t desired_size, void *ptr, size_t size){
    
    if (getenv("DEBUG_MALLOC") != NULL){
        char buf[LARGE_BUFFER];
        size_t total_size = sizeof(void *) + 2*sizeof(int);
        total_size = total_size + 
        sizeof("MALLOC: malloc()     =>   (ptr=, size=)\n");
        snprintf(buf, total_size, 
            "MALLOC: malloc(%d)     =>   (ptr=%p, size=%d)\n",  
            (int)desired_size, ptr, (int)size);
        write(STDOUT_FILENO, buf, total_size);
    }
}

void print_status_calloc(size_t old_size, size_t desired_size, 
                        void *ptr, size_t size){
    
    if (getenv("DEBUG_MALLOC") != NULL){
        char buf[LARGE_BUFFER];
        size_t total_size = sizeof(void *) + 3*sizeof(int);
        total_size = total_size + 
        sizeof("MALLOC: calloc(, )     =>   (ptr=, size=)\n");
        snprintf(buf, total_size, 
            "MALLOC: calloc(%d, %d)     =>   (ptr=%p, size=%d)\n",
            (int) old_size, (int)desired_size, ptr, (int)size);
        write(STDOUT_FILENO, buf, total_size);
    }
}

void print_status_realloc(void  *old_ptr, size_t desired_size,
                        void *ptr, size_t size){
    
    if (getenv("DEBUG_MALLOC") != NULL){
        char buf[LARGE_BUFFER];
        size_t total_size = 2*sizeof(void *) + 2*sizeof(int);
        total_size = total_size + 
        sizeof("MALLOC: realloc(, )     =>   (ptr=, size=)\n");
        snprintf(buf, total_size,
            "MALLOC: realloc(%p, %d)     =>   (ptr=%p, size=%d)\n", 
            old_ptr, (int)desired_size, ptr, (int)size);
        write(STDOUT_FILENO, buf, total_size);
    }
}

void print_status_free(void *ptr){
    
    if (getenv("DEBUG_MALLOC") != NULL){
        char buf[LARGE_BUFFER];
        size_t total_size = sizeof(void *);
        total_size = total_size + sizeof("MALLOC: free(%p)\n");
        snprintf(buf, total_size, "MALLOC: free(%p)\n", ptr);
        write(STDOUT_FILENO, buf, total_size);
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
                currentHeader -> free = FALSE;
                return (intptr_t) currentHeader + sizeOfHeader;
            }
            /*Bigger so split big block into 2 blocks*/
            if ( sizeAvailable > (desired_size + sizeOfHeader) ){
                intptr_t address = (intptr_t) currentHeader + 
                    sizeOfHeader + desired_size;

   		        Header *newHeader = (Header *) address;
                newHeader -> size =  sizeAvailable - 
                    desired_size - sizeOfHeader; /*size to store data*/
                newHeader -> free = TRUE;
                
                newHeader -> nextHeader = currentHeader -> nextHeader; 
    
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

void free(void *ptr){
    size_t sizeOfHeader = round_16(headerSize);
    intptr_t input = (intptr_t) ptr;
    Header *currentHeader;
    Header *prevHeader;
    /*Bad users ... 
    Doesn't call print_status because snprintf(3) calls*/
    if (ptr == NULL){
        return;
    }

    /*Out of heap ptr*/
    if (  (input < (intptr_t) startHeap) || 
          (input > (intptr_t) startHeap + heapSize)){
        print_status_free(NULL);
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
                                currentHeader -> nextHeader -> size 
                                + 2*sizeOfHeader;
            Header *newNext = currentHeader -> nextHeader -> nextHeader;
            prevHeader -> size = totalSize;
            prevHeader -> nextHeader = newNext; 
        }
        /*Case 2: Only previous is free*/
        else if(prevHeader != currentHeader && prevHeader -> free == TRUE){
            size_t totalSize = prevHeader -> size 
                + currentHeader -> size + sizeOfHeader;
            Header *newNext = currentHeader -> nextHeader;
            prevHeader -> size = totalSize;
            prevHeader -> nextHeader = newNext; 
        }

        /*Case 3: Only next is free*/
        else if ((currentHeader -> nextHeader != NULL) && 
              (currentHeader -> nextHeader -> free == TRUE)){
            size_t totalSize = currentHeader -> size + 
                    currentHeader -> nextHeader -> size + sizeOfHeader;
            Header *newNext = currentHeader -> nextHeader -> nextHeader;
            currentHeader -> size = totalSize;
            currentHeader -> nextHeader = newNext;
            currentHeader -> free = TRUE;

        }
        print_status_free(ptr);
    }
}


/*Calls malloc and then zeroes out everything*/
void *calloc(size_t nmemb, size_t size){
    size_t desired_size = nmemb * size;
    int count;
    void *ret_address;
    uint8_t *zero_counter;

    /*Bad users ...*/
    if (desired_size == 0 || desired_size == UINT_MAX){
        return NULL;
    }

    ret_address = malloc(desired_size);
    /*error checking*/
    if (ret_address == NULL)
        return NULL;

    zero_counter = ret_address;

    for (count = 0; count < desired_size; count++){
        *(zero_counter + count) = 0;
    }
    return ret_address;
}

/*Calls malloc and free when needed*/
void *realloc(void *ptr, size_t size){
    Header *ptrHeader = firstHeader;
    intptr_t input = (intptr_t) ptr;
    size_t sizeOfHeader = round_16(headerSize);
    size_t originalSize;

    if (ptr == NULL || size == UINT_MAX){
        return malloc(size);
    }
    if (size == 0 && ptr != NULL){
        free(ptr);
        print_status_realloc(ptr, 0, NULL, 0);
        return NULL;
    }
    else if(size == 0){
        print_status_realloc(NULL, 0, NULL, 0);
        return NULL;
    }


    /*Out of heap ptr*/
    if (  (input < (intptr_t) startHeap) || 
          (input > (intptr_t) startHeap + heapSize)){
        return NULL;
    }

    while(  input > (intptr_t) (ptrHeader ->nextHeader)){
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
            print_status_realloc(ptr, size, ptr, size);
            return (void *) ptr;
        }
        /*Case 2.2: no space for new header*/
        else{
            uint8_t *newPlace = (uint8_t *) malloc(size);
            uint8_t *copyVar =  (uint8_t *) ( (intptr_t) ptrHeader 
                                + sizeOfHeader);
            size_t count;
            
            /*error checking*/
            if (newPlace == NULL)
                return NULL;

            for (count = 0; count < size; count++){
                *(newPlace + count) = *(copyVar + count);
            }
            ptrHeader -> free = TRUE;
            print_status_realloc(ptr, size, newPlace, size);
            return newPlace;
        }
    }

    /*Case 3: original size < size*/
    if (originalSize < size){
        /*Case 3.1: Can use next header*/
        if ( (ptrHeader -> nextHeader != NULL) && 
            (ptrHeader -> nextHeader -> free == TRUE) &&
             (ptrHeader -> size + ptrHeader -> nextHeader -> size > 
                size + sizeOfHeader)){

            intptr_t address = (intptr_t) ptrHeader
                     + sizeOfHeader + size;
            size_t totalSize = originalSize + 
                    ptrHeader -> nextHeader -> size;

            Header *newHeader = (Header *) address;
            newHeader -> size =  totalSize - size; 
            newHeader -> free = TRUE;
            newHeader -> nextHeader = ptrHeader -> nextHeader -> nextHeader; 

            ptrHeader -> size = size;
            ptrHeader -> nextHeader = newHeader;
            print_status_realloc(ptr, size, ptr, size);
            return ptr;
        }
        /*Case 3.2: Have to move location*/
        else{
            uint8_t *newPlace = (uint8_t *) malloc(size);
            uint8_t *copyVar =  (uint8_t *) ((intptr_t) ptrHeader 
                            + sizeOfHeader);
            int count;

            /*error checking*/
            if (newPlace == NULL)
                return NULL;


            for (count = 0; count < size; count++){
                *(newPlace + count) = *(copyVar + count);
            }
            ptrHeader -> free = TRUE;
            print_status_realloc(newPlace, size, ptr, size);
            return newPlace;
        }
    }
    print_status_realloc(NULL, 0, NULL, 0);
    return NULL;
}


void *malloc(size_t desired_size){
    void *return_address;

     /*Bad users ...*/
    if (desired_size == 0 || desired_size == UINT_MAX){
        print_status_malloc(0, NULL, 0);
        return NULL;
    }

    if (firstTime == TRUE){
        if ((void *)(startHeap = (intptr_t)sbrk(HUNK_SIZE)) == (void *) -1){
            perror("fail sbrk");
            print_status_malloc(0, NULL, 0);
            return NULL;
        }
	    heapSize = heapSize + HUNK_SIZE;
        firstHeader = (Header *) initializeHunk(startHeap);
        lastHeader = firstHeader;
        firstTime = FALSE;
    }
    while((return_address = (void*)findBigEnoughBlock(desired_size))==NULL){
        /*Case 1: Last header in heap is free
             and we join with a new sbrk call.*/
        if (lastHeader -> free == TRUE){
            if (sbrk(HUNK_SIZE) == (void *) -1){
                perror("fail sbrk");
                print_status_malloc(0, NULL, 0);
                return NULL;
            }
	    heapSize = heapSize + HUNK_SIZE;
            lastHeader -> size = lastHeader -> size + HUNK_SIZE;
        }
        /*Case 2: Last header is full so we have to initialize 
        the new 64k block with a free header*/
        else{
            if ( (lastHeader = sbrk(HUNK_SIZE) ) == (void *) -1){
                perror("fail sbrk");
                print_status_malloc(0,NULL, 0);
                return NULL;
            }
       	    heapSize = heapSize + HUNK_SIZE;
	    initializeHunk((intptr_t)lastHeader);
        }
    }
    print_status_malloc(desired_size, return_address, desired_size);
    return return_address;
}
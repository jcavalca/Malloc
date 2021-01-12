typedef struct Header
{
        size_t size;
        uint8_t free;
        struct Header *nextHeader;
} Header;

void *my_calloc(size_t nmemb, size_t size);
void *my_malloc(size_t desired_size);
intptr_t findBigEnoughBlock(size_t desired_size);
intptr_t initializeHunk(intptr_t start);
void joinAdjacentFreeBlocks();
size_t round_16(size_t number);
/*1 bytes total*/
typedef struct Header
{
        int size;
        uint8_t free;
        struct Header *nextHeader;
} Header;


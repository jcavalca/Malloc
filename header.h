/*1 bytes total*/
typedef struct Header
{
        size_t size;
        uint8_t free;
        struct Header *nextHeader;
} Header;


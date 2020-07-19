#ifndef BUFFER_H
#define BUFFER_H

typedef struct memstruct {
    size_t block_size;
    char *current_block;
    char *current_pos;
    char *end_pos;
    } Mem;



Mem *init_mem(size_t block_size);
char *alloc_mem(Mem *mem, size_t required_bytes);
void term_mem(Mem *mem);



typedef struct singlestruct {
    size_t block_size;
    char *start_pos;
    char *current_pos;
    char *end_pos;
    } Single;



Single *init_single(size_t block_size);
char *alloc_single(Single *single, size_t required_bytes);
void term_single(Single *single);



typedef struct mapblockstruct {
    void *address;
    size_t size;
    int fp;
    } MapBlock;



typedef struct mapstruct {
    MapBlock *blocks;
    char *current_pos;
    char *end_pos;
    char *filenumber;
    size_t block_size;
    int len_blocks;
    char filename[];
    } Map;



Map *init_map(size_t default_size, const char *filename);
char *alloc_map(Map *map, size_t required_bytes);
void term_map(Map *map);



typedef struct blockstruct {
    void *address;
    size_t size;
    int entries;
    } Block;



typedef struct cyclicstruct {
    size_t default_size;
    int len_blocks;
    int current_block;
    char *current_pos;
    char *end_pos;
    Block *blocks;
    } Cyclic;



Cyclic *init_cyclic(size_t default_size);
char *alloc_cyclic(Cyclic *cyclic, size_t required_bytes);
void free_cyclic(Cyclic *cyclic, int block);
void term_cyclic(Cyclic *cyclic);



typedef struct arraystruct {
    void *address;
    size_t len;
    size_t default_number;
    size_t element_size;
    size_t allocated_elements;
    } Array;



Array *init_array(size_t default_number, size_t element_size);
int extend_array(Array *array, size_t needed);
void term_array(Array *array);

#endif

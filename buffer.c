#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "buffer.h"



#define MAPFILE_MAX_DIGITS 3
static int power(int n, int e);



static int power(int n, int e) {
    int r = 1;
    for (; e; e--) {
        r = r * n;
        }
    return r;
    }



Mem *init_mem(size_t block_size) {
    Mem *mem = NULL;
    
    if ((mem = malloc(sizeof(Mem))) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for mem struct.\n");
        return NULL;
        }
    mem->block_size = block_size,
    mem->current_block = NULL,
    mem->current_pos = NULL,
    mem->end_pos = NULL;
    return mem;
    }



char *alloc_mem(Mem *mem, size_t required_bytes) {
    char *new_block = NULL;
    char *ret = NULL;
    
    if (mem->current_pos + required_bytes > mem->end_pos) {
        if (required_bytes > mem->block_size) {
            mem->block_size = required_bytes;
            }
        if ((new_block = malloc(mem->block_size + sizeof(char *))) == NULL) {
            fprintf(stderr, "Error: Unable to allocate memory for mem buffer.\n");
            return NULL;
            }
        *((char **)new_block) = mem->current_block;
        mem->current_block = new_block;
        mem->current_pos = new_block + sizeof(char *);
        mem->end_pos = mem->current_pos + mem->block_size;
        }

    ret = mem->current_pos;
    mem->current_pos += required_bytes;
    return ret;
    }



void term_mem(Mem *mem) {
    void *this_block = NULL, *next_block = NULL;
    
    if (mem) {
        this_block = mem->current_block;
        while (this_block) {
            next_block = *((void **)this_block);
            free(this_block);
            this_block = next_block;
            }
        free(mem);
        }
    }




    
    
Single *init_single(size_t block_size) {
    Single *single = NULL;
    
    if ((single = malloc(sizeof(Single))) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for single struct.\n");
        return NULL;
        }
    single->block_size = block_size,
    single->start_pos = NULL,
    single->current_pos = NULL,
    single->end_pos = NULL;
    return single;
    }



char *alloc_single(Single *single, size_t required_bytes) {
    char *new_start = NULL, *ret = NULL;
    size_t new_size = 0, current_offset = 0;
    
    if (single->current_pos + required_bytes > single->end_pos) {
        if (required_bytes > single->block_size) {
            single->block_size = required_bytes;
            }
        new_size = (single->end_pos - single->start_pos - 1) + single->block_size; 
        if ((new_start = (char *)realloc(single->start_pos, new_size)) == NULL) {
            fprintf(stderr, "Error: Unable to allocate memory for single buffer.\n");
            return NULL;
            }
        current_offset = single->current_pos - single->start_pos;
        single->start_pos = new_start,
        single->current_pos = new_start + current_offset,
        single->end_pos = new_start + new_size;
        }

    ret = single->current_pos;
    single->current_pos += required_bytes;
    return ret;
    }



void term_single(Single *single) {
    if (single) {
        free(single->start_pos);
        free(single);
        }
    }

    
    
    


Map *init_map(size_t block_size, const char *filename) {
    Map *map = NULL;
    
    if ((map = malloc(sizeof(Map) + strlen(filename) + MAPFILE_MAX_DIGITS + 1)) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for map struct.\n");
        return NULL;
        }
    map->blocks = NULL,
    map->current_pos = NULL;
    map->end_pos = NULL,
    map->filenumber = (char *)map->filename + strlen(filename),
    map->block_size = block_size,
    map->len_blocks = 0;
    strcpy(map->filename, filename);
    return map;
    }



char *alloc_map(Map *map, size_t required_bytes) {
    MapBlock *blocks = NULL;
    char *ret;
    
    if (map->current_pos + required_bytes > map->end_pos) {
        if (required_bytes > map->block_size) {
            map->block_size = required_bytes;
            }

        if ((blocks = realloc(map->blocks, sizeof(MapBlock) * (map->len_blocks +1))) == NULL) {
            fprintf(stderr, "Error: Unable to allocate memory for mapblock struct.\n");
            return NULL;
            }
        map->blocks = blocks;
        map->blocks[map->len_blocks].address = MAP_FAILED;
        map->blocks[map->len_blocks].fp = -1;
        map->blocks[map->len_blocks].size = map->block_size;
        map->len_blocks++;
        
        if (map->len_blocks / power(10, MAPFILE_MAX_DIGITS)) {
            fprintf(stderr, "Error: Too many open map tempfiles.\n");
            return NULL;
            }
        sprintf(map->filenumber, "%i", map->len_blocks);
        
        if ((map->blocks[map->len_blocks - 1].fp = open(map->filename, O_CREAT | O_TRUNC | O_RDWR, 0600)) == -1) {
            fprintf(stderr, "Error: Unable to open map tempfile for writing.\n");
            return NULL;
            }
        if (lseek(map->blocks[map->len_blocks - 1].fp, map->block_size - 1, SEEK_SET) == -1) {
            fprintf(stderr, "Error: Unable to lseek within map tempfile.\n");
            return NULL;
            }
        if (write(map->blocks[map->len_blocks - 1].fp, "", 1) != 1) {
            printf ("Error: Unable to write to map tempfile.\n");
            return NULL;
            }
        
        if ((map->blocks[map->len_blocks - 1].address = mmap(NULL, map->block_size, PROT_READ | PROT_WRITE, MAP_SHARED, map->blocks[map->len_blocks - 1].fp, 0)) == MAP_FAILED) {
            fprintf(stderr, "Error: Unable to memory map tempfile.\n");
            return NULL;
            }

        map->current_pos = (char *)map->blocks[map->len_blocks - 1].address;
        map->end_pos = map->current_pos + map->block_size;
        }

    ret = map->current_pos;
    map->current_pos += required_bytes;
    return ret;
    }



void term_map(Map *map) {
    int i = 0;
    
    if (map) {
        for (i = 0; i < map->len_blocks; i++) {
            if (map->blocks[i].address != MAP_FAILED) {
                munmap(map->blocks[i].address, map->blocks[i].size);
                }
            if (map->blocks[i].fp != -1) {
                if (close(map->blocks[i].fp) == -1) {
                    fprintf(stderr, "Unable to close mmap tempfile.\n");
                    }
                sprintf(map->filenumber, "%i", i + 1);
                if (remove(map->filename) == -1) {
                    fprintf(stderr, "Unable to delete mmap tempfile.\n");
                    }
                }
            }
                
        free(map->blocks);
        free(map);
        }
    }

    
    
    


Cyclic *init_cyclic(size_t default_size) {
    Cyclic *cyclic = NULL;
    
    if ((cyclic = malloc(sizeof(Cyclic))) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for cyclic struct.\n");
        return NULL;
        }
    cyclic->default_size = default_size;
    cyclic->len_blocks = 0;
    cyclic->current_block = 0;
    cyclic->current_pos = NULL;
    cyclic->end_pos = NULL;
    cyclic->blocks = NULL;
    return cyclic;
    }

    
    
char *alloc_cyclic(Cyclic *cyclic, size_t required_bytes) {
    char *ret = NULL;
    
    if (cyclic->current_pos + required_bytes <= cyclic->end_pos) {
        ret = cyclic->current_pos;
        cyclic->current_pos += required_bytes;
        cyclic->blocks[cyclic->current_block].entries++;
        return ret;
        }
        
    for (cyclic->current_block = 0; cyclic->current_block < cyclic->len_blocks; cyclic->current_block++) {
        if (cyclic->blocks[cyclic->current_block].entries == 0 && cyclic->blocks[cyclic->current_block].size >= required_bytes) {
            break;
            }
        }
    
    if (cyclic->current_block == cyclic->len_blocks) {
        if (required_bytes > cyclic->default_size) {
            cyclic->default_size = required_bytes;
            }
        if ((cyclic->blocks = realloc(cyclic->blocks, sizeof(Block) * (cyclic->len_blocks + 1))) == NULL) {
            fprintf(stderr, "Error: Unable to allocate memory for block struct.\n");
            return NULL;
            }
        if ((cyclic->blocks[cyclic->len_blocks].address = (Block *)malloc(cyclic->default_size)) == NULL) {
            fprintf(stderr, "Error: Unable to allocate memory for cyclic buffer.\n");
            return NULL;
            }
        cyclic->len_blocks++;
        cyclic->blocks[cyclic->current_block].size = cyclic->default_size;
        }

    cyclic->blocks[cyclic->current_block].entries = 1;
    cyclic->current_pos = cyclic->blocks[cyclic->current_block].address + required_bytes;
    cyclic->end_pos = cyclic->blocks[cyclic->current_block].address + cyclic->blocks[cyclic->current_block].size;
    return cyclic->blocks[cyclic->current_block].address;
    }



void free_cyclic(Cyclic *cyclic, int block) {
    cyclic->blocks[block].entries--;
    }



void term_cyclic(Cyclic *cyclic) {
    if (cyclic) {
        for (cyclic->current_block = 0; cyclic->current_block < cyclic->len_blocks; cyclic->current_block++) {
            free(cyclic->blocks[cyclic->current_block].address);
            }
        free(cyclic->blocks);
        free(cyclic);
        }
    }



    
    

Array *init_array(size_t default_number, size_t element_size) {
    Array *array = NULL;
    
    if ((array = malloc(sizeof(Array))) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for array struct.\n");
        return NULL;
        }
    array->address = NULL;
    array->len = 0;
    array->default_number = default_number;
    array->element_size = element_size;
    array->allocated_elements = 0;
    return array;
    }



int extend_array(Array *array, size_t needed) {
    void *new_address = NULL;
    size_t required = 0;
    
    if (array->len + needed > array->allocated_elements) {
        required = ((array->len + needed + array->default_number - 1) / array->default_number) * array->default_number;
        if ((new_address = realloc(array->address, required * array->element_size)) == NULL) {
            fprintf(stderr, "Error: Unable to reallocate memory for array struct.\n");
            return -1;
            }
        array->address = new_address;
        memset(array->address + (array->allocated_elements * array->element_size), 0, (required - array->allocated_elements) * array->element_size);
        array->allocated_elements = required;
        }
    array->len += needed;
    return 0;
    }



void term_array(Array *array) {
    if (array) {
        free(array->address);
        free(array);
        }
    }



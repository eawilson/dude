#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "textfile.h"
#include "buffer.h"
#include "sam.h"


// 1MB 
#define HEADER_BUFFER_LEN 1024 * 1024
// 500MB
#define READ_BUFFER_LEN 512 * 1024 * 1024



static int compare_by_block_descending(const void *first, const void *second);



static int compare_by_block_descending(const void *first, const void *second) {
    AlignedRead *item1 = (AlignedRead *)first;
    AlignedRead *item2 = (AlignedRead *)second;
    
    if (item2->block > item1->block)
        return 1;
    else if (item2->block < item1->block)
        return -1;
    else
        return 0;
    }



Sam *open_sam(const char *filename, int max_reads) {
    int len_filename = 0;
    char *buffer = NULL;
    Sam *sam = NULL;
    Line *line = NULL;

    if (max_reads < 1) {
        fprintf(stderr, "Error: max_reads must be greater than zero.\n");
        goto cleanup;
        }

    len_filename = strlen(filename);
    if (len_filename < 4 || strcmp(filename + len_filename - 4 , ".sam") != 0) {
        fprintf(stderr, "Error: %s is not a sam file.\n", filename);
        goto cleanup;
        }
    
    if ((sam = calloc(1, sizeof(Sam))) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for sam struct.\n");
        goto cleanup;
        }
    sam->max_reads = max_reads;
    
    if ((sam->textfile = open_textfile(filename)) == NULL) {
        goto cleanup;
        }
    
    if ((sam->single = init_single(HEADER_BUFFER_LEN)) == NULL) {
        goto cleanup;
        }
    
    if ((sam->cyclic = init_cyclic(READ_BUFFER_LEN)) == NULL) {
        goto cleanup;
        }
    
    if ((sam->reads = malloc(sizeof(AlignedRead) * sam->max_reads)) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for alignedread struct.\n");
        goto cleanup;
        }
    
    while ((line = read_textfile(sam->textfile)) != NULL) {
        if (line->len) {
            if (line->text[0] == '@') {
                if ((buffer = alloc_single(sam->single, line->len + 1)) == NULL) {
                    goto cleanup;
                    }
                memcpy(buffer, line->text, line->len);
                buffer[line->len] = '\n';
                }
            else {
                if ((buffer = alloc_cyclic(sam->cyclic, line->len + 1)) == NULL) {
                    goto cleanup;
                    }
                memcpy(buffer, line->text, line->len);
                buffer[line->len] = '\0';
                sam->reads[0].line = buffer;
                sam->reads[0].block = sam->cyclic->current_block;
                sam->len_reads = 1;
                return sam;
                }
            }
        }
                    
    if (eof_textfile(sam->textfile)) {
        return sam;
        }

    cleanup:
    close_sam(sam);
    return NULL;
    }



void close_sam(Sam *sam) {
    if (sam != NULL) {
        close_textfile(sam->textfile);
        term_single(sam->single);
        term_cyclic(sam->cyclic);
        free(sam->reads);
        free(sam);
        }
    }



    // need to sort out return values ?finished vs error
int read_sam(Sam *sam) {
    Line *line = NULL;
    char *buffer = NULL;
    
    if (sam->len_reads < sam->max_reads) {
        while ((line = read_textfile(sam->textfile)) != NULL) {
            if (line->len) {
                if ((buffer = alloc_cyclic(sam->cyclic, line->len + 1)) == NULL) {
                    return -1;
                    }
                memcpy(buffer, line->text, line->len);
                buffer[line->len] = '\0';
                sam->reads[sam->len_reads].line = buffer;
                sam->reads[sam->len_reads].block = sam->cyclic->current_block;
                sam->len_reads++;
                if (sam->len_reads == sam->max_reads) {
                    return 0;
                    }
                }
            }
        if (!eof_textfile(sam->textfile)) {
            return -1;
            }
        }
    return 0;
    }



int resize_sam(Sam *sam, int max_reads) {
    AlignedRead *new_reads = NULL;
    
    if (max_reads < sam->len_reads) {
        fprintf(stderr, "Error: resize max_reads must not be less than len_reads.\n");
        return -1;
        }
    
    if ((new_reads = realloc(sam->reads, sizeof(AlignedRead) * max_reads)) == NULL) {
        fprintf(stderr, "Error: Unable to reallocate memory for alignedread struct.\n");
        return -1;
        }
    
    sam->reads = new_reads;
    sam->max_reads = max_reads;
    return 0;
    }
    
    
    
void flush_sam(Sam *sam) {
    qsort(sam->reads, sam->len_reads, sizeof(AlignedRead), (void *)compare_by_block_descending);
    for (sam->len_reads = 0; sam->len_reads <= sam->max_reads; sam->len_reads++) {
        if (sam->reads[sam->len_reads].block == -1) {
            break;
            }
        }
    }
    
    
        


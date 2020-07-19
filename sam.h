#ifndef SAM_H
#define SAM_H


#include <stdbool.h>
#include "buffer.h"
#include "textfile.h"



typedef struct alignedreadstruct {
    char *line;
    int block;
    } AlignedRead;



typedef struct samstruct {
    Textfile *textfile;
    Cyclic *cyclic;
    Single *single;
    AlignedRead *reads;
    int len_reads;
    int max_reads;
    } Sam;



Sam *open_sam(const char *filename, int max_reads);
int read_sam(Sam *sam);
void close_sam(Sam *sam);
int resize_sam(Sam *sam, int max_reads);
void flush_sam(Sam *sam);

#endif

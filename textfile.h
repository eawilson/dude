#ifndef TEXTFILE_H
#define TEXTFILE_H

#include <stdbool.h>
#include "zlib.h"



typedef struct textfilestruct {
    char *buffer;
    size_t len;
    size_t buffer_len;
    gzFile fp;
    char filename[];
    } Textfile;



typedef struct linestruct {
    char *text;
    size_t len;
    } Line;



Textfile *open_textfile(const char *filename);
Line *read_textfile(Textfile *textfile);
void close_textfile(Textfile *textfile);
bool eof_textfile(Textfile *textfile);

#endif

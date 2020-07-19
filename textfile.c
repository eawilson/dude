#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "zlib.h"
#include "textfile.h"



const int DEFAULT_LINE_LEN = 1024;
static const unsigned GZBUFFER_LEN = 1024 * 1024;



Textfile *open_textfile(const char *filename) {
    Textfile *textfile = NULL;
    
    if ((textfile = calloc(1, sizeof(Textfile) + strlen(filename) + 1)) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for textfile struct.\n");
        goto cleanup;
        }
    textfile->buffer_len = DEFAULT_LINE_LEN;
    textfile->len = 0;
    strcpy(textfile->filename, filename);
    if ((textfile->buffer = malloc(textfile->buffer_len)) == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for line buffer.\n");
        goto cleanup;
        }
    if ((textfile->fp = gzopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Error: Unable to open %s: %s.\n", filename, strerror(errno));
        goto cleanup;
        }
        if (gzbuffer(textfile->fp, GZBUFFER_LEN) == -1) {
            fprintf(stderr, "Error: Unable to set gzbuffer size.\n");
            goto cleanup;
            }

    return textfile;
    cleanup:
    close_textfile(textfile);
    return NULL;
    }



void close_textfile(Textfile *textfile) {
    if (textfile != NULL) {
        if (textfile->buffer != NULL) {
            free(textfile->buffer);
            textfile->buffer = NULL;
            }
        if (textfile->fp != NULL) {
            gzclose(textfile->fp);
            textfile->fp = NULL;
            }
        free(textfile);
        }
    }



Line *read_textfile(Textfile *textfile) {
    char *buffer = textfile->buffer;
    const char *error_string = NULL;
    int errnum = 0;
    
    textfile->len = 0;
    while (gzgets(textfile->fp, buffer, textfile->buffer_len - textfile->len) != NULL) {
        textfile->len = strlen(textfile->buffer);
        if (textfile->len && textfile->buffer[textfile->len - 1] == '\n') {
            textfile->len--;
            if (textfile->len && textfile->buffer[textfile->len - 1] == '\r') {
                textfile->len--;
                }
            return (Line *)textfile;
            }
        
        textfile->buffer_len += DEFAULT_LINE_LEN;
        if ((buffer = realloc(textfile->buffer, textfile->buffer_len)) == NULL) {
            fprintf(stderr, "Error: Unable to reallocate memory for line buffer.\n");
            return NULL;
            }
        textfile->buffer = buffer;
        buffer += textfile->len;
        }
    
    if (textfile->len) { // Should not happen as the last line should always end in a \n but just in case.
        if (textfile->len && textfile->buffer[textfile->len - 1] == '\r') {
            textfile->len--;
            }
        return (Line *)textfile;
        }

    if (!gzeof(textfile->fp)) {
        error_string = gzerror(textfile->fp, &errnum);
        fprintf (stderr, "Error: %s reading %s.\n", error_string, textfile->filename);
        }
        
    return NULL;
    }
        
        
        
bool eof_textfile(Textfile *textfile) {
    return (bool)gzeof(textfile->fp);
    }

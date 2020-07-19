#include <stdio.h>
#include <stdbool.h>

#include "dude.h"
#include "buffer.h"



void write_stats(Options *options, Stats *stats) {
    FILE *fp = NULL;
    int size = 0;
    bool first = true;
    
    if ((fp = fopen(options->stats_filename, "w")) == NULL) {
        fprintf(stderr, "Error: Unable to open %s for writing.\n", options->stats_filename);
        goto cleanup;
        }
    
    fprintf(fp, "{\n");
    
    fprintf(fp, "\"file_size\": %ld,\n", stats->file_size / 1024 / 1024);
    fprintf(fp, "\"reads\": %i,\n", stats->total_reads);
    fprintf(fp, "\"inadequate_reads\": %i,\n", stats->inadequate_reads);
    fprintf(fp, "\"sized_reads\": %i,\n", stats->sized_reads);
    fprintf(fp, "\"families\": %i,\n", stats->families);
    if (options->umi_len) {
        fprintf(fp, "\"size_families\": %i,\n", stats->size_families);
        }
    if (stats->sequencing_bases) {
        fprintf(fp, "\"sequencing_error_rate\": %.4f,\n", (float)stats->sequencing_errors / stats->sequencing_bases);
        }
    if (stats->pcr_bases) {
        fprintf(fp, "\"pcr_error_rate\": %.4f,\n", (float)stats->pcr_errors / stats->pcr_bases);
        }
        
    first = true;
    fprintf(fp, "\"family_sizes\": {");
    for (size = 1; size < stats->family_sizes->len; ++size) {
        if (((int *)stats->family_sizes->address)[size]) {
            if (!first) {
                fprintf(fp, ",");
                }
            first = false;
            fprintf(fp, "\n                \"%i\": %i", size, ((int *)stats->family_sizes->address)[size]);
            }
        }
    fprintf(fp, "\n                },\n");

    first = true;
    fprintf(fp, "\"fragment_sizes\": {");
    for (size = 1; size < stats->fragment_sizes->len; ++size) {
        if (((int *)stats->fragment_sizes->address)[size]) {
            if (!first) {
                fprintf(fp, ",");
                }
            first = false;
            fprintf(fp, "\n                  \"%i\": %i", size, ((int *)stats->fragment_sizes->address)[size]);
            }
        }
    fprintf(fp, "\n                  }\n");
        
//     first = true;
//     fprintf(fp, "\"qtwenty_bases\": {");
//     for (size = 0; size < stats->qtwenty_bases->len; ++size) {
//         if (((int *)stats->qtwenty_bases->address)[size]) {
//             if (!first) {
//                 fprintf(fp, ",");
//                 }
//             first = false;
//             fprintf(fp, "\n                  \"%i\": %i", size, ((int *)stats->qtwenty_bases->address)[size]);
//             }
//         }
//     fprintf(fp, "\n                  }\n");
        
    fprintf(fp, "}\n");
            
    cleanup:
    if (fp != NULL && fclose(fp) != 0) {
        fprintf(stderr, "Error: Unable to close %s.\n", options->output_filename);
        }
    }



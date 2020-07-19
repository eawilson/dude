#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <getopt.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dude.h"


#define FASTQ 0
#define FASTQGZ 1
#define SAM 2



static int arg_to_int(const char *arg);
static bool endswith(const char *text, const char *suffix);



int main (int argc, char **argv) {
    /*
     * 
     */
    char **filenames = NULL;
    const char *filetypes[3] = {".fastq", ".fastq.gz", ".sam"};
    const int len_filetypes = 3;
    int len_filenames = 0, i = 0, filetype = 0, ret = -1;
    
    Options options = {.error_correction = FULL_CORRECTION,
                       .allowed = 3,
                       .interleaved = false,
                       .max_consequtive_ns = 2,
                       .min_read_length = 50,
                       .umi_len = 0,
                       .umi_stem = 0,
                       .pcr_free = false,
                       .output_filename = NULL,
                       .stats_filename = "stats.json"};

    Stats stats;
    memset(&stats, 0, sizeof(Stats));
    if ((stats.fragment_sizes = init_array(1024, sizeof(int))) == NULL) {
        goto cleanup;
        }
    if ((stats.family_sizes = init_array(50, sizeof(int))) == NULL) {
        goto cleanup;
        }
    if ((stats.qtwenty_bases = init_array(100, sizeof(int))) == NULL) {
        goto cleanup;
        }
    
    // Variables needed by getopt_long
    int option_index = 0, c = 0;
    static struct option long_options[] = {{"output", required_argument, 0, 'o'},
                                           {"stats", required_argument, 0, 's'},
                                           {"error-correction", required_argument, 0, 'e'},
                                           {"pcr-free", no_argument, 0, 'n'},
                                           
                                           // fsastq options
                                           {"interleaved", no_argument, 0, 'p'},
                                           {"allowed", required_argument, 0, 'a'},
                                           {"max-consequtive-ns", required_argument, 0, 'm'},
                                           {"min-read-length", required_argument, 0, 'l'},
                                           {"umi", required_argument, 0, 'u'},
                                           
                                           // sam options
                                           {0, 0, 0, 0}};

    // Parse optional arguments
    while (c != -1) {
        c = getopt_long (argc, argv, "o:s:e:npa:m:l:u:", long_options, &option_index);

        switch (c) {
            case 'o':
                options.output_filename = optarg;
                break;
                
            case 's':
                options.stats_filename = optarg;
                break;
            
            case 'e': // NEED TO CODE THIS*******************************
                break;
                
            case 'p':
                options.interleaved = true;
                break;
                
            case 'a':
                // allowed = number of different bases allowed to still consider 2 sequences the same
                options.allowed = arg_to_int(optarg);
                break;

            case 'm':
                options.max_consequtive_ns = arg_to_int(optarg);
                break;                
                
            case 'l':
                options.min_read_length = arg_to_int(optarg);
                break;                
                
            case 'u':
                if (strcmp(optarg, "thruplex")) {
                    options.umi_len = 6;
                    options.umi_stem = 11;
                    }
                else if (strcmp(optarg, "prism")) {
                    options.umi_len = 8;
                    }
                else {
                    fprintf(stderr, "Error: Unsupported umi: %s\n.", optarg);
                    goto cleanup;
                    }
                break;
            
            case 'n':
                options.pcr_free = true;
                break;
                
            case '?':
                // unknown option, getopt_long already printed an error message.
                goto cleanup;
            }
        }

    
    filenames = argv + optind;
    len_filenames = argc - optind;
    if (len_filenames == 0) {
        fprintf(stderr, "Error: No input files supplied.\n");
        goto cleanup;
        }
    
    for (filetype = 0; filetype < len_filetypes; filetype++) {
        for (i = 0; i < len_filenames; i++) {
            if (!endswith(filenames[i], filetypes[filetype])) {
                break;
                }
            }
        if (i == len_filenames) {
            break;
            }
        }
        
    if (filetype == FASTQ || filetype == FASTQGZ) {
        if (!options.interleaved && len_filenames % 2) {
            fprintf(stderr, "An even number of paired fastqs are required.\n");
            goto cleanup;
            }
        if (options.output_filename == NULL) {
            options.output_filename = "output.interleaved.fastq";
            }
        dedupe_fastq(filenames, len_filenames, &options, &stats);
        }
    else if (filetype == SAM) {
        if (len_filenames > 1) {
            fprintf(stderr, "Error: Only one sam file is allowed as input.\n");
            goto cleanup;
            }
        if (options.output_filename == NULL) {
            options.output_filename = "output.unsorted.sam";
            }
        dedupe_sam(filenames[0], &options, &stats);
        }

    else {
        fprintf(stderr, "Error: Ambiguous input filetypes.\n");
        goto cleanup;
        }
        
    write_stats(&options, &stats);
    
    ret = 0;
    cleanup:
    term_array(stats.fragment_sizes);
    term_array(stats.family_sizes);
    term_array(stats.qtwenty_bases);
    return ret;
    }



static int arg_to_int(const char *arg) {
    int retval = -1;
    char *ptr = NULL;
    
    retval = strtol(arg, &ptr, 10);
    if (retval < 0 || retval > INT_MAX || errno || *ptr != '\0') {
        fprintf(stderr, "'%s' is not a valid integer argument.\n", arg);
        retval = -1;
        }
    return (int)retval;
    }



static bool endswith(const char *text, const char *suffix) {
    int offset = strlen(text) - strlen(suffix);
    return (offset >= 0 && strcmp(text + offset, suffix) == 0);
    }





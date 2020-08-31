#ifndef DUDE_H
#define DUDE_H

#include <stdbool.h>
#include <stddef.h>
#include "buffer.h"

#define SIGNIFICANT_PHRED_DIFFERENCE 10
#define R1 0
#define R2 1

#define N_ONLY_READ -3
#define SHORT_READ -2
#define IDENTICAL_READ -1

#define UMI_NONE 0
#define UMI_THRUPLEX 1
#define UMI_PRISM 2

#define NO_CORRECTION 0
#define INTRAREAD_ONLY 1
#define FULL_CORRECTION 2



typedef struct readstruct {
    char *seq;
    char *qual;
    char *umi;
    char *umi_qual;
    int len;
    } Read;



typedef struct readpairstruct {
    Read read[2];
    char *name;
    int family;
    int prevfamily; // Used as temp starage during mergematrix family creation
    short fragment_size;
    short intraread_errors;
    //short quality_improvement;
    short overlap;
    } ReadPair;



typedef struct optionsstruct {
    int error_correction;
    int allowed;
    bool interleaved;
    int max_consequtive_ns;
    int min_read_length;
    int min_overlap;
    int umi_len;
    int umi_stem;
    bool pcr_free;
    char *output_filename;
    char *stats_filename;
    } Options;



typedef struct statsstruct {
    long int file_size;             // Size of input file uncompressed execluding newline characters
    int total_reads;                // All reads including short and N-only
    int inadequate_reads;           // Short reads or too many consequtive Ns
    int sized_reads;                // Reads that can be sized
    int size_families;              // Number of families by size/position/sequence alone
    int families;                   // Number of famiies
    int sequencing_bases;           //
    int sequencing_errors;          //
    int pcr_bases;                  //
    int pcr_errors;                 //
    Array *fragment_sizes;          //
    Array *family_sizes;            //
    Array *qtwenty_bases;           //
    } Stats;

    
    
typedef struct pairedfastqstruct {
    Array *readpairs;
    Options *options;
    Stats *stats;
    Mem *sequence;
    Map *quality;
    } PairedFastq;

    
    
typedef struct mergematrixstruct {
    int first_match;
    int second_match;
    int swap;
    } MergeMatrix;
                    


// read_fastqs.c
int dedupe_fastq(char **filenames, int len_filenames, Options *options, Stats *stats);
void dedupe_sam(char *filename, Options *options, Stats *stats);

// stats.c
void write_stats(Options *options, Stats *stats);


#endif

#define _GNU_SOURCE

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
#include <ctype.h>

#include "dude.h"
#include "textfile.h"
#include "buffer.h"



static const size_t SEQUENCE_BUFFER_BLOCKSIZE = 1024 * 1024 * 512; // 500MB
static const size_t QUALITY_BUFFER_BLOCKSIZE = 1024 * 1024 * 1024; // 1GB
static const size_t READPAIRS_ELEMENTS = 1024 * 1024; // 1,000,000 elements

static const char *index2base = "NACGT";
static const int base2index[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0};

static const char reversebase[256] = {
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'T', 'N', 'G', 'N', 'N', 'N', 'C', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'A', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
    'N', 'N', 'N', 'N', 'N', 'N'};



static PairedFastq read_fastq(char **filenames, int len_filenames, Options *options, Stats *stats);
static bool inadequate_read(ReadPair *readpair, Options *options);
// static int count_qtwenty_bases(ReadPair *readpair, Stats *stats);

static void size_and_remove_umis(PairedFastq fq);
static void reverse(char *start, int len);
static void reversecomplement(char *start, int len);
static bool do_they_overlap(ReadPair *read, int min_overlap, int allowed);

static void correct_intraread_errors(PairedFastq fq);

static int assign_families(PairedFastq fq);
static void brute_assign_families(ReadPair *bin_start, ReadPair *bin_end, int *current_family, int allowed);
static bool are_they_duplicates(ReadPair *readpair1, ReadPair *readpair2, int allowed);
static int count_families(PairedFastq fq);

static void assign_umi_families(PairedFastq fq);

static int collapse_families(PairedFastq fq);

static int compare_by_fragment_size_descending(const void *one, const void *two);
static int compare_by_family(const void *one, const void *two);
static int compare_by_short_sequence(const void *first, const void *second, const void *offset);


    
int dedupe_fastq(char **filenames, int len_filenames, Options *options, Stats *stats) {
    int ret = -1;

    // Read fastqs
    PairedFastq fq = read_fastq(filenames, len_filenames, options, stats);
    if (fq.readpairs == NULL) {
        goto cleanup;
        }
    fprintf(stderr, "Total reads = %i\nInadequate reads = %i\nFile size = %ld MB.\n", fq.stats->total_reads, fq.stats->inadequate_reads, fq.stats->file_size / 1024 / 1024);
    
//     for (i = 0; i < fq.readpairs->len; ++i) {
//         count_qtwenty_bases((ReadPair *)(fq.readpairs->address) + i, stats);
//         }
    
    // Size fragments and remove umis
    size_and_remove_umis(fq);
    fprintf(stderr, "Sized %i reads (%i%% of %i).\n", fq.stats->sized_reads, fq.stats->sized_reads * 100 / fq.stats->total_reads, fq.stats->total_reads);
    
    if (fq.options->error_correction >= INTRAREAD_ONLY) {
        correct_intraread_errors(fq);
        }
    
    // Group reads into families
    if (assign_families(fq) == -1)
        goto cleanup;
    fprintf(stderr, "Created %i size families from %i reads.\n", fq.stats->families, fq.stats->total_reads);
    
    // Subgroup families based on umis
    if (fq.options->umi_len) {
        assign_umi_families(fq);
        fprintf(stderr, "Created %i umi families from %i reads.\n", fq.stats->families, fq.stats->total_reads);
        }

        // Collapse families into single consensus reads and write output to file
    if (collapse_families(fq) == -1) {
        goto cleanup;
        }

    ret = 0;
    cleanup:
    term_array(fq.readpairs);
    term_mem(fq.sequence);
    term_map(fq.quality);
    return ret;
    }



static PairedFastq read_fastq(char **filenames, int len_filenames, Options *options, Stats *stats) {
    PairedFastq fq = {.readpairs = NULL,
                      .sequence = NULL,
                      .quality = NULL,
                      .options = options,
                      .stats = stats};
    
    Textfile *fastqfiles[2] = {NULL, NULL};
    Line *line = NULL;
    ReadPair *readpair = NULL;
    int fastq = 0, read = R1, lineno = 0, i = 0;
    char *pos = NULL;
    size_t bytes = 0;
    
    if ((fq.sequence = init_mem(SEQUENCE_BUFFER_BLOCKSIZE)) == NULL ||
        (fq.quality = init_map(QUALITY_BUFFER_BLOCKSIZE, "quality.temp")) == NULL ||
        (fq.readpairs = init_array(READPAIRS_ELEMENTS, sizeof(ReadPair))) == NULL) {
        goto cleanup;
        }
    
    for (fastq = 0; fastq < len_filenames;) {
        for (read = R1; read <= R2; ++read) {
            if (options->interleaved && read == R2) {
                fastqfiles[R2] = fastqfiles[R1];
                }
            else if ((fastqfiles[read] = open_textfile(filenames[fastq++])) == NULL) {
                goto cleanup;
                }
            }

        for (;;) {
            for (i = 0; i < 8; ++i) {
                read = i / 4;
                lineno = i % 4;
                if ((line = read_textfile(fastqfiles[read])) == NULL) {
                    break;
                    }
                fq.stats->file_size += line->len;

                if (i == 0) {
                    if (extend_array(fq.readpairs, 1) == -1) {
                        goto cleanup;
                        }
                    readpair = (ReadPair *)fq.readpairs->address + (fq.readpairs->len - 1);
                    }
                
                switch (lineno) {
                    case 0: // Identifier
                        if ((pos = memchr(line->text, ' ', line->len)) != NULL) {
                            bytes = pos - line->text;
                            }
                        else {
                            bytes = line->len;
                            }
                        line->text[bytes] = '\0';
                            
                        if (read == R1) {
                            if ((readpair->name = alloc_map(fq.quality, bytes + 1)) == NULL) {
                                goto cleanup;
                                }
                            memcpy(readpair->name, line->text, bytes + 1);
                            }
                        else if (read == R2 && strcmp(readpair->name, line->text) != 0) {
                            fprintf(stderr, "Error: Mismatched reads %s and %s.\n", readpair->name, line->text);
                            goto cleanup;
                            }
                        break;
                        
                    case 1: // Sequence
                        if ((readpair->read[read].seq = alloc_mem(fq.sequence, line->len + 2) + 1) == NULL + 1) {
                            goto cleanup;
                            }
                        memcpy(readpair->read[read].seq, line->text, line->len);
                        readpair->read[read].seq[line->len] = '\0';
                        readpair->read[read].len = line->len;
                        break;
            
                    case 3: // Quality
                        if (line->len != readpair->read[read].len) {
                            fprintf(stderr, "Error: Sequence and quality differ in length for read %s.\n", readpair->name);
                            goto cleanup;
                            }
                        if ((readpair->read[read].qual = alloc_map(fq.quality, line->len + 2) + 1) == NULL + 1) {
                            goto cleanup;
                            }
                        memcpy(readpair->read[read].qual, line->text, line->len);
                        readpair->read[read].qual[line->len] = '\0';
                        break;

                    }
                }
            
            if (line == NULL) {
                break;
                }
                
            if (inadequate_read(readpair, options)) {
                ++stats->inadequate_reads;
                fq.readpairs->len -=1;
                }
            }
        
        if (read != R1 || lineno != 0 || read_textfile(fastqfiles[R2]) != NULL) {
            fprintf(stderr, "Error: Truncated fastq.\n");
            goto cleanup;
            }
        else if (!eof_textfile(fastqfiles[R1]) || !eof_textfile(fastqfiles[R2])) {
            goto cleanup;
            }
        
        close_textfile(fastqfiles[R1]);
        close_textfile(fastqfiles[R2]);
        }
    
    fq.stats->total_reads = fq.readpairs->len + fq.stats->inadequate_reads;
    return fq;

    cleanup:
    close_textfile(fastqfiles[R1]);
    close_textfile(fastqfiles[R2]);
    term_array(fq.readpairs);
    fq.readpairs = NULL;
    return fq;
    }



static bool inadequate_read(ReadPair *readpair, Options *options) {
    /* Remove reads consisting of only Ns in either the forward or reverse read as these will match everything.
     * Also removes reads with less than MINIMUM_NON_N_BASES of non N bases.
     */ 
    int i = 0, consequtive = 0, read = R1;
         
    for (read = R1; read <= R2; ++read) {
        if (readpair->read[read].len < options->min_read_length) {
            return true;
            }
        
        consequtive = 0;
        for (i = 0; i < readpair->read[read].len; i++) {
            if (readpair->read[read].seq[i] == 'N') {
                if (++consequtive > options->max_consequtive_ns) {
                    return true;
                    }
                }
            else {
                consequtive = 0;
                }
            }
        }
    return false;
    }

    
    
// static int count_qtwenty_bases(ReadPair *readpair, Stats *stats) {
//     int i = 0, read = R1, bases = 0;
//     
//     for (read = R1; read <= R2; ++read) {
//         for (i = 0; i < readpair->read[read].len; ++i) {
//             if (readpair->read[read].qual[i] - 33 < 21) {
//                 ++bases;
//                 }
//             }
//         }
// 
//     if (bases + 1 > stats->qtwenty_bases->len) {
//         if (extend_array(stats->qtwenty_bases, bases + 1 - stats->qtwenty_bases->len) == -1) {
//             return -1;
//             }
//         }
//     
//     if (bases > 240) {
//         printf("%s %s\n%s %s\n\n", readpair->read[R1].seq, readpair->read[R2].seq, readpair->read[R1].qual, readpair->read[R1].qual);
//         }
//     
//     ++(((int *)stats->qtwenty_bases->address)[bases]);
//     return 0;
//     }
    
    

static void size_and_remove_umis(PairedFastq fq) {
    /* Fragment size paired reads, Correct sequencing errors if phred difference large enough or 'N' them out otherwise.
     * If thruplex then trim the umis including if there has been runthrough sequencing into the stem / umi at the other end.
     */
    int readthrough = 0, read = R1;
    int umi_len = fq.options->umi_len;
    int umi_stem = fq.options->umi_stem;
    ReadPair *readpair = NULL;

    for (readpair = (ReadPair *)fq.readpairs->address; readpair < (ReadPair *)fq.readpairs->address + fq.readpairs->len; ++readpair) {
        reversecomplement(readpair->read[R2].seq, readpair->read[R2].len);
        reverse(readpair->read[R2].qual, readpair->read[R2].len);
        
        fq.stats->sized_reads += do_they_overlap(readpair, 50, fq.options->allowed);
        
        reversecomplement(readpair->read[R2].seq, readpair->read[R2].len);
        reverse(readpair->read[R2].qual, readpair->read[R2].len);            

        if (umi_len) {
            for (read = R1; read <= R2; ++read) {
                if (readpair->fragment_size) {
                    readthrough = readpair->overlap + umi_len + umi_stem - readpair->read[!read].len;
                    if (readthrough > 0) { // Can be negative.
                        readpair->read[read].len -= readthrough;
                        readpair->read[read].seq[readpair->read[read].len] = '\0';
                        readpair->read[read].qual[readpair->read[read].len] = '\0';
                        readpair->overlap -= readthrough;
                        }
                    readpair->fragment_size -= 2 * (umi_len + umi_stem);
                    }
            
                readpair->read[read].umi = readpair->read[read].seq;
                readpair->read[read].umi_qual = readpair->read[read].qual;
                if (!umi_stem) {
                    memmove(--readpair->read[read].umi, readpair->read[read].seq, umi_len);
                    memmove(--readpair->read[read].umi_qual, readpair->read[read].qual, umi_len);
                    }
                readpair->read[read].umi[umi_len] = '\0';
                readpair->read[read].seq += umi_len + umi_stem;
                readpair->read[read].umi_qual[umi_len] = '\0';
                readpair->read[read].qual += umi_len + umi_stem;
                readpair->read[read].len -= umi_len + umi_stem;
                }
            }
        }
    }



static void reverse(char *start, int len) {
    char *end = start + len - 1;
    char temp = '\0';

    while (end > start) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
        }
    }



static void reversecomplement(char *start, int len) {
    char *end = start + len - 1;
    char temp = '\0';

    while (end > start) {
        temp = reversebase[*(unsigned char *)start];
        *start = reversebase[*(unsigned char *)end];
        *end = temp;
        start++;
        end--;
        }
    if (start == end) {
        *start = reversebase[*(unsigned char *)end];
        }
    }



static bool do_they_overlap(ReadPair *readpair, int min_overlap, int allowed) {
    int read1_start = 0, best_read1_start = 0, mismatches = 0, best = allowed + 1, ret = 0;
    char *seq1 = NULL, *seq2 = NULL;
    if (min_overlap > readpair->read[R1].len || min_overlap > readpair->read[R2].len)
        return 0;
    
    if (readpair->read[R1].len <= readpair->read[R2].len)
        read1_start = 0;
    else 
        read1_start = readpair->read[R1].len - readpair->read[R2].len;
    
    // read1_start = index position in read1 to start comparing with read2[0]
    for(; read1_start < readpair->read[R1].len - min_overlap + 1; read1_start++) {
        seq1 = readpair->read[R1].seq + read1_start;
        seq2 = readpair->read[R2].seq;
        mismatches = 0;
        while (*seq1 != '\0' && *seq2 != '\0') {
            if (*seq1 != *seq2 && *seq1 != 'N' && *seq2 != 'N') {
                ++mismatches;
                if (mismatches > allowed)
                    break;
                }
            seq1++;
            seq2++;
            }
        // Don't need to store best as we quit after first match, but just in case we ever want to search to the end...
        if (mismatches < best) {
            best = mismatches;
            best_read1_start = read1_start;
            break;
            }
        }
        
    if (best <= allowed) {
        readpair->fragment_size = readpair->read[R2].len + best_read1_start;
        readpair->overlap = readpair->read[R1].len - best_read1_start;
        ret = 1;
        }
    return ret;
    }



void correct_intraread_errors(PairedFastq fq) {
    ReadPair *readpair = (ReadPair *)fq.readpairs->address, *end_readpairs = (ReadPair *)fq.readpairs->address + fq.readpairs->len;
    int i1 = 0, i2 = 0;
    
    for (; readpair < end_readpairs; ++readpair) {
        if (readpair->overlap) {
            i1 = readpair->read[R1].len - readpair->overlap;
            i2 = readpair->read[R2].len - 1;
            while (i1 < readpair->read[R1].len) {
                if (readpair->read[R1].seq[i1] != reversebase[*(unsigned char *)(readpair->read[R2].seq + i2)]) {
                    if (readpair->read[R1].seq[i1] != 'N' && readpair->read[R2].seq[i2] != 'N') {
                        ++readpair->intraread_errors;
                        }
                        
                    if (readpair->read[R1].qual[i1] > (int)readpair->read[R2].qual[i2] + SIGNIFICANT_PHRED_DIFFERENCE || readpair->read[R2].seq[i2] == 'N') {
                        readpair->read[R2].seq[i2] = reversebase[*(unsigned char *)(readpair->read[R1].seq + i1)];
                        readpair->read[R2].qual[i2] = readpair->read[R1].qual[i1];
                        }
                    else if (readpair->read[R2].qual[i2] > (int)readpair->read[R1].qual[i1] + SIGNIFICANT_PHRED_DIFFERENCE || readpair->read[R1].seq[i1] == 'N') {
                        readpair->read[R1].seq[i1] = reversebase[*(unsigned char *)(readpair->read[R2].seq + i2)];
                        readpair->read[R1].qual[i1] = readpair->read[R2].qual[i2];
                        }
                    else {
                        readpair->read[R1].seq[i1] = 'N';
                        readpair->read[R1].qual[i1] = '!';
                        readpair->read[R2].seq[i2] = 'N';
                        readpair->read[R2].qual[i2] = '!';
                        }
                    }
                ++i1;
                --i2;
                }
                
            fq.stats->sequencing_bases += 2 * readpair->overlap;
            fq.stats->sequencing_errors += readpair->intraread_errors;
            }
        }
    }



static int assign_families(PairedFastq fq) {
    int biggest_bin = 0, current_family = 0, temp_family = 0, family = 0, max_family = 0, n = 0, i = 0, incomplete = 0, merge_required = 0;

    ReadPair *bin_start = NULL, *bin_end = NULL, *subbin_start = NULL, *subbin_end = NULL;
    MergeMatrix *mergematrix = NULL;
    
    qsort((ReadPair *)fq.readpairs->address, fq.readpairs->len, sizeof(ReadPair), (void *)compare_by_fragment_size_descending);

    // Calculate size of largest bin to allocate memory.
    bin_start = (ReadPair *)fq.readpairs->address;
    while (bin_start < (ReadPair *)fq.readpairs->address + fq.readpairs->len) {
        for (bin_end = bin_start + 1; bin_end < (ReadPair *)fq.readpairs->address + fq.readpairs->len; bin_end++) {
            if (bin_end->fragment_size != bin_start->fragment_size)
                break;
            }
        if (bin_end - bin_start > biggest_bin)
            biggest_bin = bin_end - bin_start;
        bin_start = bin_end;
        }
    mergematrix = malloc(biggest_bin * sizeof(MergeMatrix));
    if (mergematrix == NULL) {
        fprintf(stderr, "Unable to allocate memory for merge matrix.\n");
        return -1;
        }
        
    // Do the actual family assessment.
    bin_start = (ReadPair *)fq.readpairs->address;
    while (bin_start < (ReadPair *)fq.readpairs->address + fq.readpairs->len) {
        for (bin_end = bin_start + 1; bin_end < (ReadPair *)fq.readpairs->address + fq.readpairs->len; bin_end++) {
            if (bin_end->fragment_size != bin_start->fragment_size)
                break;
            }
            
        if (bin_end - bin_start < 2000) {
            brute_assign_families(bin_start, bin_end, &current_family, fq.options->allowed);
            }
            
        else {
            for (n = 0; n < fq.options->allowed + 4; n++) {
                temp_family = 0;
                qsort_r(bin_start, bin_end - bin_start, sizeof(ReadPair), (void *)compare_by_short_sequence, &n);
                subbin_start = bin_start;
                while (subbin_start < bin_end) {
                    for (subbin_end = subbin_start + 1; subbin_end < bin_end; subbin_end++) {
                        if (compare_by_short_sequence(subbin_start, subbin_end, &n) != 0)
                            break;
                        }
                    brute_assign_families(subbin_start, subbin_end, &temp_family, fq.options->allowed);
                    subbin_start = subbin_end;
                    }
                
                if (n == 0) { // Don't need to merge families on first iteration.
                    for (i = 0; bin_start + i < bin_end; i++) {
                        bin_start[i].prevfamily = bin_start[i].family;
                        bin_start[i].family = 0;
                        }
                    }
                    
                else {
                    do {
                        memset(mergematrix, 0, biggest_bin * sizeof(MergeMatrix));
                        incomplete = 0;
                        merge_required = 0;

                        for (i = 0; bin_start + i < bin_end; i++) {
                            family = bin_start[i].family;
                            if (mergematrix[family].first_match == 0)
                                mergematrix[family].first_match = bin_start[i].prevfamily;
                            else if (mergematrix[family].first_match == bin_start[i].prevfamily)
                                ;
                            else if (mergematrix[family].second_match == 0) {
                                mergematrix[family].second_match = bin_start[i].prevfamily;
                                if (mergematrix[mergematrix[family].first_match].swap == 0) {
                                    mergematrix[mergematrix[family].second_match].swap = mergematrix[family].first_match;
                                    merge_required = 1;
                                    }
                                else
                                    incomplete = 1;
                                }
                            else if (mergematrix[family].second_match == bin_start[i].prevfamily)
                                ;
                            else
                                incomplete = 1;
                            }

                        if (merge_required) {
                            //printf("merging\n");
                            for (i = 0; bin_start + i < bin_end; i++) {
                                family = bin_start[i].prevfamily;
                                if (mergematrix[family].swap != 0) {
                                    bin_start[i].prevfamily = mergematrix[family].swap;
                                    }
                                }
                            }
                            
                        } while (incomplete);

                    for (i = 0; bin_start + i < bin_end; i++) {
                        bin_start[i].family = 0;
                        }
                    } 
                }
            max_family = 0;
            for (i = 0; bin_start + i < bin_end; i++) {
                family = bin_start[i].prevfamily + current_family;
                bin_start[i].family = family;
                if (family > max_family)
                    max_family = family;
                }
            current_family = max_family;
            }
            
        //fprintf(stderr, "Fragment size = %i, bin size = %i\n", bin_start->fragment_size, (int)(bin_end - bin_start));
        //printf("%i,%i\n", bin_start->fragment_size, (int)(bin_end - bin_start));
        bin_start = bin_end;
        }
    free(mergematrix);
    fq.stats->size_families = count_families(fq);
    fq.stats->families = fq.stats->size_families;
    return 0;
    }
    
    
    
static void brute_assign_families(ReadPair *bin_start, ReadPair *bin_end, int *current_family, int allowed) {
    ReadPair *readpair, *readpair2, *readpair3;
    int joined_family;
    //printf("bin size = %i, allowed = %i\n", bin_end - bin_start, allowed);
    for (readpair = bin_start; readpair < bin_end; readpair++) {
        if (readpair->family == 0)
            readpair->family = ++(*current_family);
        
        for(readpair2 = readpair + 1; readpair2 < bin_end; readpair2++) {
            //printf("comparing\n%s\n%s\n", readpair->read[R1].seq, readpair2->read[R1].seq);
            if (are_they_duplicates(readpair, readpair2, allowed)) {
                //printf("match\n");
                if (readpair2->family == 0)
                    readpair2->family = readpair->family;
                else if (readpair2->family != readpair->family) {
                    joined_family = readpair2->family;
                    for(readpair3 = bin_start; readpair3 < bin_end; readpair3++) {
                        if (readpair3->family == joined_family)
                            readpair3->family = readpair->family;
                        }
                    }
                }
            }
        }
    }



static bool are_they_duplicates(ReadPair *readpair1, ReadPair *readpair2, int allowed) {
    int mismatches = 0, read = R1;
    char *base1 = NULL, *base2 = NULL, *end1 = NULL;
    
    for (read = R1; read <= R2; ++read) {
        base1 = readpair1->read[read].seq;
        base2 = readpair2->read[read].seq;    
        end1 = base1 + (readpair1->read[read].len < readpair2->read[read].len ? readpair1->read[read].len : readpair2->read[read].len);
        while (base1 < end1) {
            if (*base1 != *base2 && *base1 != 'N' && *base2 != 'N') {
                mismatches++;
                if (mismatches > allowed)
                    return 0;
                }
            ++base1;
            ++base2;
            }
        }
    return 1;
    }

    

static int count_families(PairedFastq fq) {
    int families = 0, last_family = -1;
    ReadPair *readpair = NULL;
    
    qsort((ReadPair *)fq.readpairs->address, fq.readpairs->len, sizeof(ReadPair), (void *)compare_by_family);
    
    for (readpair = (ReadPair *)fq.readpairs->address; readpair < (ReadPair *)fq.readpairs->address + fq.readpairs->len; readpair++) {
        if (readpair->family != last_family) {
            families++;
            last_family = readpair->family;
            }
        }
    return families;
    }

    

static void assign_umi_families(PairedFastq fq) {
    ReadPair *readpair = NULL, *readpair2 = NULL, *readpair3 = NULL, *bin_start = NULL, *bin_end = NULL, *last_readpair = NULL;
    int current_family = 0, joined_family = 0;
    
    qsort((ReadPair *)fq.readpairs->address, fq.readpairs->len, sizeof(ReadPair), (void *)compare_by_family);
    last_readpair = (ReadPair *)fq.readpairs->address + fq.readpairs->len - 1;
    current_family = last_readpair->family;
    
    for (bin_start = (ReadPair *)fq.readpairs->address; bin_start <= last_readpair; bin_start = bin_end) {
        bin_start->prevfamily = bin_start->family;
        for (bin_end = bin_start + 1; bin_end <= last_readpair; ++bin_end) {
            if (bin_end->family != bin_start->family)
                break;
            bin_end->prevfamily = bin_end->family;
            bin_end->family = 0;
            }
            
        for (readpair = bin_start; readpair < bin_end; readpair++) {
            if (readpair->family == 0)
                readpair->family = ++current_family;
            for(readpair2 = readpair + 1; readpair2 < bin_end; readpair2++) {
                if (memcmp(readpair->read[R1].umi, readpair2->read[R1].umi, fq.options->umi_len) == 0 ||
                    memcmp(readpair->read[R2].umi, readpair2->read[R2].umi, fq.options->umi_len) == 0) {
                    if (readpair2->family == 0)
                        readpair2->family = readpair->family;
                    else if (readpair2->family != readpair->family) {
                        joined_family = readpair2->family;
                        for(readpair3 = bin_start; readpair3 < bin_end; readpair3++) {
                            if (readpair3->family == joined_family)
                                readpair3->family = readpair->family;
                            }
                        }
                    }
                }
            }
        }
        
    fq.stats->families = count_families(fq);
    }



static int collapse_families(PairedFastq fq) {
    ReadPair *bin_start = NULL, *bin_end = NULL, *readpair = NULL;
    FILE *fp = NULL;
    int j = 0, i = 0, needed = 0, counts[5] = {0, 0, 0, 0, 0}, read = R1, ret = -1, family_size = 0, consensus_base = 0, overlap = 0;
    char *consensus_seq = NULL, *consensus_qual = NULL;
    size_t allocated_len = 0, max_len = 0;
    
    if ((fp = fopen(fq.options->output_filename, "w")) == NULL) {
        fprintf(stderr, "Error: Unable to open %s for writing.\n", fq.options->output_filename);
        goto cleanup;
        }
    
    qsort((ReadPair *)fq.readpairs->address, fq.readpairs->len, sizeof(ReadPair), (void *)compare_by_family);
    for (bin_start = (ReadPair *)fq.readpairs->address; bin_start < (ReadPair *)fq.readpairs->address + fq.readpairs->len; bin_start = bin_end) {
        overlap = bin_start->overlap;
        for (bin_end = bin_start + 1; bin_end < (ReadPair *)fq.readpairs->address + fq.readpairs->len; bin_end++) {
            if (bin_end->family != bin_start->family)
                break;
            if (bin_end->overlap < overlap) {
                overlap = bin_end->overlap;
                }
            }
        family_size = bin_end - bin_start;
        
//         if (family_size != 205) {
//             continue;
//             for (readpair = bin_start; readpair < bin_end; readpair++) {
//                 fprintf(stderr, "%s  %s\n", readpair->read[R1].seq, readpair->read[R2].seq);
//                 }
//             fprintf(stderr, "\n");
//             }
//         
        if (bin_start->fragment_size + 1 > fq.stats->fragment_sizes->len) {
            if (extend_array(fq.stats->fragment_sizes, bin_start->fragment_size + 1 - fq.stats->fragment_sizes->len) == -1) {
                return -1;
                }
            }
        ++(((int *)fq.stats->fragment_sizes->address)[bin_start->fragment_size]);

        if (family_size + 1 > fq.stats->family_sizes->len) {
            if (extend_array(fq.stats->family_sizes, family_size + 1 - fq.stats->family_sizes->len) == -1) {
                return -1;
                }
            }
        ++(((int *)fq.stats->family_sizes->address)[family_size]);

        
        if (family_size == 1) {
            for (read = R1; read <= R2; ++read) {
                fprintf(fp, "%s YF:i:1", bin_start->name);
                if (bin_start->fragment_size) {
                    fprintf(fp, "\tYS:i:%i", (int)bin_start->fragment_size);
                    }
                fprintf(fp, "\n%s\n+\n%s\n", bin_start->read[read].seq, bin_start->read[read].qual);
                }
            }
        else {
            if (overlap) {
                fq.stats->pcr_bases += overlap * family_size;
                }

            // Allocate memory for consensus sequence.
            max_len = 0;
            for (read = R1; read <= R2; read++) {
                for (readpair = bin_start; readpair < bin_end; readpair++) {
                    if (readpair->read[read].len > max_len) {
                        max_len = readpair->read[read].len;
                        }
                    }
                }

                if (max_len > allocated_len) {
                if ((consensus_qual = realloc(consensus_seq, (max_len + 1) * 2)) == NULL) {
                    goto cleanup;
                    }
                allocated_len = max_len;
                consensus_seq = consensus_qual;
                consensus_qual += max_len + 1;
                }
            
            // Generate consensus sequence and quality.
            needed = (6 * family_size) / 10;
            for (read = R1; read <= R2; ++read) {
                consensus_seq[max_len] = '\0';
                consensus_qual[max_len] = '\0';
                
                for (i = 0; i < max_len; ++i) {
                    memset(counts, 0, sizeof(int) * 5);
                    for (readpair = bin_start; readpair < bin_end; ++readpair) {
                        if (i < readpair->read[read].len) {
                            ++counts[base2index[*(unsigned char *)(readpair->read[read].seq + i)]];
                            }
                        }
                    
                    consensus_base = 1;
                    for (j = 2; j < 5; ++j) {
                        if (counts[j] > counts[consensus_base]) {
                            consensus_base = j;
                            }
                        }
                    
                    if (counts[consensus_base] > needed) {
                        consensus_seq[i] = index2base[consensus_base];                        
                        for (readpair = bin_start; readpair < bin_end; ++readpair) {
                            if (i < readpair->read[read].len && readpair->read[read].seq[i] == index2base[consensus_base] && readpair->read[read].qual[i] > consensus_qual[i]) {
                                consensus_qual[i] = readpair->read[read].qual[i];
                                }
                            }
                        }
                    else {
                        consensus_seq[i] = 'N';
                        consensus_qual[i] = '!';
                        }
                    
                    if (counts[consensus_base] < family_size && overlap && bin_start->read[read].len - i - 1 < overlap) {
                        for (j = 1; j < 5; ++j) {
                            if (j != consensus_base) {
                                fq.stats->pcr_errors += counts[j];
                                }
                            }
                        }
                    
                    }
                
                fprintf(fp, "%s YF:i:%i", bin_start->name, family_size);
                if (bin_start->fragment_size) {
                    fprintf(fp, "\tYS:i:%i", (int)bin_start->fragment_size);
                    }
                fprintf(fp, "\n%s\n+\n%s\n", consensus_seq, consensus_qual);
                }
            }
        }
    ret = 0;
    cleanup:
    if (fp != NULL && fclose(fp) != 0) {
        fprintf(stderr, "Error: Unable to close %s.\n", fq.options->output_filename);
        return -1;
        }
    free(consensus_seq);
    return ret;
    }



// static int write_read(ReadPair *readpair, FILE *fp) {
//     Read *read = readpair->read, *end_reads = readpair->read + 2;
//     char sep = ' ';
//     
//     for (; read < end_reads; ++read) {
//         fprintf(fp, "%s", readpair->name);
//         if (read->umi) {
//             fprintf(fp, "%cRX:Z:%s-%s\tQX:Z:%s %s", sep, readpair->read[R1].umi,  readpair->read[R2].umi,  readpair->read[R1].umi_qual,  readpair->read[R2].umi_qual);
//             sep = '\t';
//             }
//         if (readpair->overlap) {
//             fprintf(fp, "%cYI:i:%i", sep, (int)readpair->intraread_errors);
//             sep ='\t';
//             }
//         
//         fprintf(fp, "\n%s\n+\n%s\n", read->seq, read->qual);
//         }
//     }
    
    
    
    
    
    
    
    
    
    
// DO WE NEED TO WORRY ABOUT OVERSHOOTING ON SHORT READS? .. Probanly not
static int compare_by_short_sequence(const void *first, const void *second, const void *offset) {
    ReadPair *item1 = (ReadPair *)first;
    ReadPair *item2 = (ReadPair *)second;
    int index = 10 + (*((int *)offset) * 6);
    int comp;

    comp = memcmp(item1->read[R1].seq + index, item2->read[R1].seq + index, 6);
    if (comp == 0) {
        comp = memcmp(item1->read[R2].seq + index, item2->read[R2].seq + index, 6);
        }
    return comp;
    }



static int compare_by_fragment_size_descending(const void *first, const void *second) {
    ReadPair *item1 = (ReadPair*)first;
    ReadPair *item2 = (ReadPair*)second;
    
    if (item2->fragment_size > item1->fragment_size)
        return 1;
    else if (item2->fragment_size < item1->fragment_size)
        return -1;
    else
        return 0;
    }
    


static int compare_by_family(const void *first, const void *second) {
    ReadPair *item1 = (ReadPair*)first;
    ReadPair *item2 = (ReadPair*)second;
    
    if (item2->family < item1->family)
        return 1;
    else if (item2->family > item1->family)
        return -1;
    else
        return 0;
    }

    
    

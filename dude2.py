import csv
import pdb
import argparse
from colections import defaultdict

QNAME = 0
FLAG = 1
RNAME = 2
POS = 3
CIGAR = 6



def maim()
    parser = argparse.ArgumentParser()
    parser.add_argument('sam_in', nargs="+", help="Input SAM file.")
    parser.add_argument("-o", "--output", help="Output SAM file name.", dest="sam_out", default="output.sam")
    args = parser.parse_args()

    singles = {}
    pairs = defaultdict(list)
    chrom = None

    with opem(args.sam_out, "wt") as f_out:
        writer = csv.writer(f_out, delimiter="\t")
        with open(sam_in, "rt") as f_in:
            reader = csv.readerf_in, delimiter="\t")
            for row in reader:
                flag = row[FLAG] = int(row[FLAG])
                row[POS] = int(row[POS])
                
                # not a primary read or unmapped
                if not 0x900 & flag or 0x4 & flag:
                    continue
                    
                if chrom != row[RNAME]:
                    deduplicate(f_out, pairs.values())
                    chrom = row[RNAME]
                    pairs = defaultdict(list)
                
                identifier = row[QNAME].split("|")
                if identifier not in singles:
                    singles[identifier] = row
                    
                else:
                    row1 = singles.pop(identifier)
                    start = row1[POS]
                    stop = row[POS] + cigar_len(row[CIGAR]) - 1
                    pairs[(start, stop)] += [(row1, row)]
                    


def deduplicate(f_out, size_families):
    for umi_families in size_families:
        for family in ():
        
            
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    
    
    
def cigar_complexity(cigar):
    complexity = 0
    for char in cigar:
        if char == "D" or char == "I":
            complexity += 2
        elif char == "S":
            complexity += 1
    return complexity



def cigar_len(cigar):
    length = 0
    while cigar:
        for i in range(0, len(cigar):
            if not cigar[i].isnumeric():
                break
        bases = int(cigar[:i-1])
        op = cigar[i]
        cigar = cigar[i+1:]
    
        if op in ("MDN=X"):
            length += bases
    return length
        
    
    
if __name__ == "__main__":
    main()

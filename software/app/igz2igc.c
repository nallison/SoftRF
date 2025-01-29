/*
 *  Decompress an IGC file as done in SoftRF for writing to flash
 *
 *  Usage:  igz2igc infile outfile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* buffer sizes */
#define BSIZE  255

/* line length limits for error checking (not including CRLF) */
#define LONGLINE  128

char *inbuf;
char *outbuf;
long nlines = 0;

char outfilename[256];

#define EMPTY_B_RECORD   "B~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
const char *emptybrecord;
char brecord[40];
int tpos[22] = {1,2,3,4,7,8,9,10,14,15,16,17,18,19,23,24,25,26,27,30,31,32};
int bpos[12] = { 5, 6, 11, 12, 13, 20, 21, 22, 28, 29, 33, 34 };

void decompress(infile, outbuf, outfile)
    FILE *infile;
    char *outbuf;
    FILE *outfile;
{
    char *p = outbuf;
    char *t = outbuf + BSIZE;
    int state = 0;
    int i = 0;
    while (p < t) {
        char c;
        if (fread(&c, 1, 1, infile) == 0)
            break;
        if (state == 0) {    // beginning of a line
            p = outbuf;
            if (c == 0x0A) {
                state = 0xAA;
            } else if (c == 0x0C) {
                strcpy(p,"LPLT");
                p += 4;
                state = 0xAA;
            } else {
                int opr = (c & 0xE0);
                int idx = (c & 0x1F);
                if (opr == 0xA0) {
                    ++brecord[tpos[idx]];
                } else if (opr == 0xE0) {
                    --brecord[tpos[idx]];
                } else if (opr == 0xC0) {
                    state = tpos[idx];
                    // will read next byte into template at that position
                } else {
                    // read a compressed B-record
                    i = 0;
                    char c1 = '0' + (c & 0x0F);
                    char c2 = '0' + ((c & 0xF0) >> 4);
                    brecord[bpos[i++]] = c1;
                    brecord[bpos[i++]] = c2;
                    state = 0xBB;
                }
            }
        } else if (state == 0xAA) {  // verbatim
            if (c == '\n') {      // end of line
                *p++ = '\r';
                *p++ = '\n';
                state = 0;
                fwrite(outbuf, 1, (p-outbuf), outfile);
                ++nlines;
            } else {
                *p++ = c;
            }
        } else if (state == 0xBB) {   // decompress B-record
            char c1 = '0' + (c & 0x0F);
            char c2 = '0' + ((c & 0xF0) >> 4);
            brecord[bpos[i++]] = c1;
            brecord[bpos[i++]] = c2;
            if (i == 12) {
                fwrite(brecord, 1, 37, outfile);
                ++nlines;
                state = 0;
            }
        } else if (state <= 32) {   // template byte
            brecord[state] = c;
            state = 0;
        }
    }
}

void
main (argc, argv)
    int    argc;
    char     **argv;
{
    char c;
    FILE *infile, *outfile;

    if (argc < 2) {
        printf("Usage: igz2igc infile.IGZ [outfile.IGC]\n");
        exit(1);
    }

    /* allocate buffers */

    if ((inbuf = malloc(BSIZE)) == NULL
    || (outbuf = malloc(BSIZE)) == NULL) {
        fprintf (stderr, "error allocating buffers\n");
        exit(1);
    }

    /* open files */

    infile = fopen (argv[1], "rb");
    if (infile == NULL) {
        fprintf (stderr,
            "error opening input file '%s'\n", argv[1]);
        exit(1);
    }

    if (argc > 2) {
        strcpy(outfilename, argv[2]);
    } else {
        strcpy(outfilename, argv[1]);
        int fnlen = strlen(outfilename);
        if (outfilename[fnlen-1] != 'Z') {
            printf("Usage: igz2igc infile.IGZ [outfile.IGC]\n");
            exit(1);
        }
        outfilename[fnlen-1] = 'C';
    }

    outfile = fopen (outfilename, "wb");
    if (outfile == NULL) {
        fprintf (stderr,
            "error opening output file '%s'\n", argv[2]);
        exit(1);
    }

    emptybrecord = EMPTY_B_RECORD;
    strcpy(brecord, emptybrecord);

    /* read input and convert to output */
    decompress (infile, outbuf, outfile);

    fclose (infile);
    fclose (outfile);

    printf("output %d lines\n", nlines);
    exit (0);
}

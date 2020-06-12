
#include "huffman.h"
HUFFBITS dmask = 1 << (sizeof(HUFFBITS) * 8 - 1);
struct huffcodetab ht[HTN]; /* array of all huffcodtable headers */
/* read the huffman decoder table */
int read_decoder_table(FILE* fi)
{
    int n, i, nn, t;
    unsigned int v0, v1;
    char command[100], line[100];
    for (n = 0; n < HTN; n++)
    {
        /* .table number treelen xlen ylen linbits */
        do
        {
            fgets(line, 99, fi);
        } while ((line[0] == '#') || (line[0] < ' '));
        sscanf_s(line, "%s %s %u %u %u %u", command,100, ht[n].tablename,3,&ht[n].treelen, &ht[n].xlen, &ht[n].ylen, &ht[n].linbits);
        if (strcmp(command, ".end") == 0)
            return n;
        else if (strcmp(command, ".table") != 0)
        {
            fprintf(stderr, "huffman table %u data corrupted\n", n);
            return -1;
        }
        ht[n].linmax = (1 << ht[n].linbits) - 1;
        sscanf_s(ht[n].tablename, "%u", &nn);
        if (nn != n)
        {
            fprintf(stderr, "wrong table number %u\n", n);
            return (-2);
        }
        do
        {
            fgets(line, 99, fi);
        } while ((line[0] == '#') || (line[0] < ' '));
        sscanf_s(line, "%s %u", command,100, &t);
        if (strcmp(command, ".reference") == 0)
        {
            ht[n].ref = t;
            ht[n].val = ht[t].val;
            ht[n].treelen = ht[t].treelen;
            if ((ht[n].xlen != ht[t].xlen) ||
                (ht[n].ylen != ht[t].ylen))
            {
                fprintf(stderr, "wrong table %u reference\n", n);
                return (-3);
            };
            while ((line[0] == '#') || (line[0] < ' '))
            {
                fgets(line, 99, fi);
            }
        }
        else if (strcmp(command, ".treedata") == 0)
        {
            ht[n].ref = -1;
            ht[n].val = (unsigned char(*)[2])calloc(2 * (ht[n].treelen), sizeof(unsigned char));
            if ((ht[n].val == NULL) && (ht[n].treelen != 0))
            {
                fprintf(stderr, "heaperror at table %d\n", n);
                exit(-10);
            }
            for (i = 0; i < ht[n].treelen; i++)
            {
                fscanf_s(fi, "%x %x", &v0, &v1);
                ht[n].val[i][0] = (unsigned char)v0;
                ht[n].val[i][1] = (unsigned char)v1;
            }
            fgets(line, 99, fi); /* read the rest of the line */
        }
        else
        {
            fprintf(stderr, "huffman decodertable error at table %d\n", n);
        }
    }
    return n;
}
/* do the huffman-decoding */
/* note! for counta,countb -the 4 bit value is returned in y, discard x */
int huffman_decoder(struct huffcodetab* h, int* x, int* y, int* v, int* w)
{
    HUFFBITS level;
    int point = 0;
    int error = 1;
    level = dmask;
    if (h->val == NULL)
        return 2;
    /* table 0 needs no bits */
    if (h->treelen == 0)
    {
        *x = *y = 0;
        return 0;
    }
    /* Lookup in Huffman table. */
    do
    {
        if (h->val[point][0] == 0)
        { /*end of tree*/
            *x = h->val[point][1] >> 4;
            *y = h->val[point][1] & 0xf;
            error = 0;
            break;
        }
        if (hget1bit())
        {
            while (h->val[point][1] >= MXOFF)
                point += h->val[point][1];
            point += h->val[point][1];
        }
        else
        {
            while (h->val[point][0] >= MXOFF)
                point += h->val[point][0];
            point += h->val[point][0];
        }
        level >>= 1;
    } while (level || (point < ht->treelen))
        ;
    /* Check for error. */
    if (error)
    { /* set x and y to a medium value as a simple concealment */
        printf("Illegal Huffman code in data.\n");
        *x = (h->xlen - 1 << 1);
        *y = (h->ylen - 1 << 1);
    }
    /* Process sign encodings for quadruples tables. */
    if (h->tablename[0] == '3' && (h->tablename[1] == '2' || h->tablename[1] == '3'))
    {
        *v = (*y >> 3) & 1;
        *w = (*y >> 2) & 1;
        *x = (*y >> 1) & 1;
        *y = *y & 1;
        /* v, w, x and y are reversed in the bitstream.
switch them around to make test bistream work. */
/* {int i=*v; *v=*y; *y=i; i=*w; *w=*x; *x=i;} MI */
        if (*v) if (hget1bit() == 1) *v = -*v;
        if (*w)
            if (hget1bit() == 1)
                *w = -*w;
        if (*x)
            if (hget1bit() == 1)
                *x = -*x;
        if (*y)
            if (hget1bit() == 1)
                *y = -*y;
    }
    /* Process sign and escape encodings for dual tables. */
    else
    {
        /* x and y are reversed in the test bitstream.
Reverse x and y here to make test bitstream work. */
/* removed 11/11/92 -ag
{int i=*x; *x=*y; *y=i;}
*/
        if (h->linbits)
            if ((h->xlen - 1) == *x)
                *x += hgetbits(h->linbits);
        if (*x)
            if (hget1bit() == 1)
                *x = -*x;
        if (h->linbits)
            if ((h->ylen - 1) == *y)
                *y += hgetbits(h->linbits);
        if (*y)
            if (hget1bit() == 1)
                *y = -*y;
    }
    return error;
}
#pragma once
#ifndef _HUFFMAN_

#define _HUFFMAN_
#include <string.h>
#include <stdlib.h>
#include "common.h"

#define HUFFBITS unsigned long int
#define HTN 34
#define MXOFF 250
struct huffcodetab {
	char tablename[3]; /*string, containing table_description */
	unsigned int xlen;  /*max. x-index+        */
	unsigned int ylen; /*max. y-index+ */
	unsigned int linbits; /*number of linbits */
	unsigned int linmax; /*max number to be stored in linbits */
	int ref; /*a positive value indicates a reference*/
	HUFFBITS* table; /*pointer to array[xlen][ylen] */
	unsigned char* hlen; /*pointer to array[xlen][ylen] */
	unsigned char(*val)[2];/*decoder tree */
	unsigned int treelen; /*length of decoder tree */
};


/* 0..31 Huffman code table 0..31 */
/* 32,33 count1-tables */

int read_decoder_table(FILE* fi);
int huffman_decoder(struct huffcodetab* h, int* x, int* y, int* v, int* w);
#endif
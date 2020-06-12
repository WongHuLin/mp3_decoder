#pragma once
#ifndef _COMMON_

#define _COMMON_
#include <stdio.h>
#include <math.h>

#define MPG_MD_STEREO           0
#define MPG_MD_JOINT_STEREO     1
#define MPG_MD_DUAL_CHANNEL     2
#define MPG_MD_MONO             3

#define FALSE 0
#define TRUE 1

#define HAN_SIZE 512
#define ALIGNING 8

#define SCALE    32768
#define SBLIMIT    32
#define SSLIMIT    18
#define BUFFER_SIZE 4096
#define MAX_LENGTH 32
#define MINIMUM 4

#define SYNC_WORD  (long)0xfff
#define SYNC_WORD_LENGTH  12

#define BINARY 0
#define READ_MODE 0

#define PI     3.14159265358979
#define PI64    PI/64
#define PI4     PI/4

#define MIN(A, B)   ((A) < (B) ? (A) : (B))


#define BUFSIZE 4096
typedef struct
{
    int version;
    int lay;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
}layer, * the_layer;

typedef struct
{
    layer* header;
    int actual_mode;
    int stereo;
    int jsbound;
    int sblimit;
}frame_params;

typedef struct
{
    FILE* pt;
    unsigned char* buf;
    int buf_size;
    long totbit;
    int buf_byte_idx;
    int buf_bit_idx;
    int mode;
    int eob;
    int eobs;
    char format;
}Bit_stream_struc;

struct gr_info_s
{
    unsigned part2_3_length;
    unsigned big_values;
    unsigned global_gain;
    unsigned scalefac_compress;
    unsigned window_switching_flag;
    unsigned block_type;
    unsigned mixed_block_flag;
    unsigned table_select[3];
    unsigned subblock_gain[3];
    unsigned region0_count;
    unsigned region1_count;
    unsigned preflag;
    unsigned scalefac_scale;
    unsigned count1table_select;
};

typedef struct
{
    unsigned main_data_begin;
    unsigned private_bits;
    struct
    {
        unsigned scfsi[4];
        struct gr_info_s gr[2];
    }ch[2];
}III_side_info_t;

typedef struct iii_scalefac_t
{
    int l[23];
    int s[3][13];
}III_scalefac_t[2];


FILE* OpenTableFile(char* name);

void WriteHdr(frame_params* fr_ps);

void* mem_alloc(unsigned long block, char* item);

void alloc_buffer(Bit_stream_struc* bs, int size);

void desalloc_buffer(Bit_stream_struc* bs);

void open_bit_stream_r(Bit_stream_struc* bs, char* bs_filenam, int size);

void close_bit_stream_r(Bit_stream_struc* bs);

int end_bs(Bit_stream_struc* bs);

unsigned long sstell(Bit_stream_struc* bs);

void refill_buffer(Bit_stream_struc* bs);

unsigned int get1bit(Bit_stream_struc* bs);

unsigned long getbits(Bit_stream_struc* bs, int N);

int seek_sync(Bit_stream_struc* bs, unsigned long sync, int N);

int js_bound(int lay, int m_ext);

void hdr_to_frps(frame_params* fr_ps);

void hputbuf(unsigned int val, int N);

unsigned long hsstell();

unsigned long hgetbits(int N);

unsigned int hget1bit();

void rewindNbits(int N);

void rewindNbytes(int N);

#endif
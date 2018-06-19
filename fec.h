//
//  fec.h
//  fec
//
//  Created by fanzy on 14-9-7.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#ifndef fec_fec_h
#define fec_fec_h

#include <stdint.h>

#include "para.h"
#include "matrix.h"
#include "decoder.h"

#define FEC_BLOCK_SIZE_DEFAULT 1316
#define FEC_BLOCK_SIZE_MAX 1440

#define FEC_FLAG_USERMEM 1
#define FEC_FLAG_DATA 2
typedef struct {
    uint8_t* d;
    int flag;
} fec_data_t;

#define FEC_BLOCK_MIN 4
#define FEC_BLOCK_MAX 8192

typedef struct {
    fec_para_t     p;
    fec_matrix_t   m;
    fec_decoder_t  d;
    int k;
    int n;
    int cnt;
    int blockSize;
    fec_data_t data[0];
} fec;

fec* fecCreate(int k, int n, int blockSize);
void fecClose(fec*);

//int fecGetBlockSize(fec*, int*);
//int fecSetBlockSize(fec*, int);

void fecDataAppend(fec*, int, void*);
void fecBuffAppend(fec*, int, void*);
void* fecData(fec*, int);

int fecEncode(fec*);
int fecDecode(fec*);



#endif

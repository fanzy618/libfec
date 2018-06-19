//
//  fec.c
//  fec
//
//  Created by fanzy on 14-9-7.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "fec.h"

fec* fecCreate(int k, int n, int blockSize)
{
    fec* f = malloc(sizeof(fec) + sizeof(fec_data_t) * n);
    memset(f, 0, sizeof(fec) + sizeof(fec_data_t) * n);
    f->k = k;
    f->n = n;
    f->blockSize = blockSize;
    return f;
}

void fecClose(fec* f)
{
    if(f) {
        for (int i = 0; i < f-> n; i++) {
            if (f->data[i].d != NULL && (f->data[i].flag & FEC_FLAG_USERMEM) == 0) {
                free(f->data[i].d);
            }
        }
        _fecDecoderClose(&f->d);
        _fecMatrixClose(&f->m);
        free(f);
    }
}

static void fecAppendWithFlag(fec* f, int idx, void* d, int flag)
{
    assert(idx >=0 && idx < f->n);
    assert(d != NULL);
    assert(f->data[idx].d == NULL);
    
    f->data[idx].d = d;
    f->data[idx].flag = flag;
}

void fecDataAppend(fec* f, int idx, void* d)
{
    fecAppendWithFlag(f, idx, d, FEC_FLAG_DATA | FEC_FLAG_USERMEM);
    f->cnt += 1;
}

void fecBuffAppend(fec* f, int idx, void* d)
{
    fecAppendWithFlag(f, idx, d, FEC_FLAG_USERMEM);
}

void* fecData(fec* f, int idx) {
    assert(f != NULL);
    assert(idx >= 0 && idx < f->n);
    return f->data[idx].d;
}

static int prepare(fec* f)
{
    // Alloc memeory
    for (int i = 0; i < f->n; i++) {
        if (!f->data[i].d) {
            void* d = malloc(f->blockSize);
            fecAppendWithFlag(f, i, d, 0);
        }
    }
    
    _fecParaInit(&f->p, f->k);
    _fecMatrixAInit(f);
//    _fecMatrixPrint("Init A completed1", &f->m);
    return _fecGetSymbols(f);
}

int fecEncode(fec* f)
{
    assert(f->cnt >= f->k);
    assert(f->cnt <= f->n);
    if (prepare(f) != 0) {
        return -1;
    }
    
    fec_decoder_t *dec = &f->d;
    fec_para_t *p = &f->p;
    
    for (int i = f->k; i < f->n; i++) {
        int a,b,d;
        _fecLtTriple(p, i, &d, &a, &b);

        for (; b >= p->l; b = (a+b) % p->p) {}
        memcpy(f->data[i].d, dec->data[b], f->blockSize);
        
        if (d > p->l) {
            d = p->l;
        }
        
        for (int j = 1; j < d; j++) {
            for (b = (a+b) % p->p; b >= p->l; b = (a+b) % p->p) {}
            for (int k = 0; k < f->blockSize; k++) {
                f->data[i].d[k] ^= dec->data[b][k];
            }
        }
    }

    return 0;
}

int fecDecode(fec* f)
{
    assert(f->cnt >= f->k);
    assert(f->cnt <= f->n);
    if(prepare(f) != 0) {
        return -1;
    }
    
    fec_decoder_t *dec = &f->d;
    fec_para_t *p = &f->p;
    
    for (int i = 0; i < f->k; i++) {
        if (f->data[i].flag & FEC_FLAG_DATA) {
            continue;
        }
        
        int a,b,d;
        _fecLtTriple(p, i, &d, &a, &b);
        
        for (; b >= p->l; b = (a+b) % p->p) {}
        memcpy(f->data[i].d, dec->data[b], f->blockSize);
        
        if (d > p->l) {
            d = p->l;
        }
        
        for (int j = 1; j < d; j++) {
            for (b = (a+b) % p->p; b >= p->l; b = (a+b) % p->p) {}
            for (int k = 0; k < f->blockSize; k++) {
                f->data[i].d[k] ^= dec->data[b][k];
            }
        }
    }
    return 0;
}



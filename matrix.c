//
//  matrix.c
//  fec
//
//  Created by fanzy on 14-9-8.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "fec.h"
#include "const.h"

static void fecMatrixCreate(fec *f)
{
    fec_matrix_t *m = &f->m;
    fec_para_t *p = &f->p;
    
    m->row = p->s + p->h + f->cnt;
    m->col = p->l;
    m->m = malloc(m->row * sizeof(void*));
    
    for (int i=0; i < m->row; i++) {
        m->m[i] = malloc(m->col * sizeof(uint8_t));
        memset(m->m[i], 0, sizeof(uint8_t) * m->col);
    }
}



// RFC 5053 5.4.2.4.2
static void initSRow(fec *f)
{
    fec_matrix_t *m = &f->m;
    fec_para_t *p = &f->p;
    
    //G_LDPC
    int a, b;
    for (int i=0; i < p->k; i++) {
        a = 1 + (int)floor((double)i / (double)p->s) % (p->s - 1);
        b = i % p->s;
        m->m[b][i] = 1;
        // Repeat twice
        b = (b+a) % p->s;
        m->m[b][i] = 1;
        b = (b+a) % p->s;
        m->m[b][i] = 1;
    }
    //I_S
    for (int i=0; i < p->s; i++) {
        m->m[i][i+p->k] = 1;
    }
}

static int countOne(int n)
{
    assert(n >= 0);
    int cnt = 0;
    for (; n > 0; n >>= 1) {
        cnt += n & 1;
    }
    return cnt;
}

static void getSeqM(int j, int k, int* s, int cnt)
{
    assert(cnt >= j);
    int n = 0;
    for (int i = 0; n < j; i++) {
        int g = i ^ (int)floor((double)i / 2.0);
        if (countOne(g) == k) {
            s[n] = g;
            n++;
        }
    }
}

// RFC 5053 5.4.2.4.2
static void initHRow(fec *f)
{
    fec_matrix_t *m = &f->m;
    fec_para_t *p = &f->p;
    
    int seqM[256] = {0};
    getSeqM(p->k + p->s, (int)ceil((double)p->h / 2.0), seqM, sizeof(seqM));
    for (int i=0; i < p->h; i++) {
        //G_Half
        for (int j = 0; j < p->k + p->s; j++) {
            if ((1<<i) & seqM[j]) {
                m->m[p->s + i][j] = 1;
            }
        }
        
        //I_H
        m->m[p->s+i][p->k+p->s+i] = 1;
    }
}

//5.4.4.1
//Rand[X, i, m] = (V0[(X + i) % 256] ^ V1[(floor(X/256)+ i) % 256]) % m
static uint32_t lt_rand(int x, int i, int m)
{
	uint32_t a = lt_rand_v0[(x + i) % 256];
	uint32_t b = lt_rand_v1[(int)(floor(x / 256.0))+i % 256];
//    printf("rand: %u %u %u\n", x, a, b);
	return (a ^ b) % m;
}

//5.4.4.2
static int lt_degree(int v)
{

	const int f[] = {0, 10241, 491582, 712794, 831695, 948446, 1032189, 1048576};
	const int d[] = {0, 1, 2, 3, 4, 10, 11, 40};
	const int cnt = sizeof(f) / sizeof(f[0]);
    if (v == 0) {
		return 0;
	}
    
	for(int i=0; i < cnt; i++) {
		if (v < f[i]) {
			return d[i];
		}
	}
	//Error
	return 0;
}

//5.4.4.4
void _fecLtTriple(fec_para_t *p, int id, int *d, int *a, int *b)
{
    const uint32_t Q = 65521;
    uint32_t A = (53591 + lt_systematic_index[p->k - 4] * 997) % Q;
    uint32_t B = 10267 * (lt_systematic_index[p->k - 4] + 1) % Q;
    uint32_t Y = (B + id * A) % Q;
    uint32_t v = lt_rand(Y, 0, 1048576);
//    printf("lt: %u %u %u %u %u\n", Q, A, B, Y, v);
    // Output
    *d = lt_degree(v);
    *a = 1 + lt_rand(Y, 1, p->p - 1);
    *b = lt_rand(Y, 2, p->p);
}

//5.4.4.3
static void initLTRow(fec *f)
{
    fec_matrix_t *m = &f->m;
    fec_para_t *p = &f->p;
    int row = p->s + p->h;
    
    for (int i=0; i < f->n; i++) {
        if ((f->data[i].flag & FEC_FLAG_DATA) == 0) {
            continue;
        }
        int a,b,d;
        _fecLtTriple(p, i, &d, &a, &b);
//        printf("%s: row=%d %d %d %d %d\n", __func__, row, i, d, a, b);
        
        for (; b >= p->l; b = (a+b) % p->p) {}
        m->m[row][b] = 1;
        
        if (d > p->l) {
            d = p->l;
        }
        
        for (int j = 1; j < d; j++) {
            for (b = (a+b) % p->p; b >= p->l; b = (a+b) % p->p) {}
            m->m[row][b] = 1;
        }
        
        row++;
    }
}

void _fecMatrixPrint(const char* title, fec_matrix_t *m)
{
    printf("====  %s  ====\n", title);
    for (int i = 0; i < m->row; i++) {
        printf("%d:\t", i);
        for (int j = 0; j < m->col; j++) {
            printf("%d ", m->m[i][j]);
        }
        printf("\n");
    }
}

void _fecMatrixAInit(void* p)
{
    fec* f = (fec*)p;
    fecMatrixCreate(f);
    
    initSRow(f);
    initHRow(f);
    initLTRow(f);
    
}

void _fecMatrixClose(fec_matrix_t* m)
{
    for (int i = 0; i < m->row; i++) {
        free(m->m[i]);
    }
    free(m->m);
}





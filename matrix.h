//
//  matrix.h
//  fec
//
//  Created by fanzy on 14-9-9.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#ifndef fec_matrix_h
#define fec_matrix_h

#include <stdint.h>

typedef struct {
    int row, col;
    //uint8_t* buff;
    uint8_t** m;
} fec_matrix_t;

void _fecMatrixAInit(void* p);
void _fecMatrixClose(fec_matrix_t* m);
void _fecMatrixPrint(const char* title, fec_matrix_t *m);

void _fecLtTriple(fec_para_t *p, int id, int *d, int *a, int *b);

#endif

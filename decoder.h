//
//  decoder.h
//  fec
//
//  Created by fanzy on 14-9-11.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#ifndef fec_decoder_h
#define fec_decoder_h

#include <stdint.h>

typedef struct {
    //intermediate symbols
    uint8_t **data;
    //for memory alloc and free
    uint8_t *buff;
    int *c, *d;
    int m;
    int i, u;
} fec_decoder_t;

typedef struct _node {
    int row;
    struct _node* next;
} fec_node_t;

typedef struct _comp {
    int cnt;
    fec_node_t* nodes;
    struct _comp* next;
} fec_comp_t;

typedef struct {
    // The max number of colomns is 8419 when k is 8192.
    fec_comp_t* col[8419];
    fec_comp_t* header;
} fec_graph_t;

int _fecGetSymbols(void*);
void _fecDecoderClose(fec_decoder_t *d);

#endif

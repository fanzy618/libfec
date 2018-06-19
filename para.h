//
//  para.h
//  fec
//
//  Created by fanzy on 14-9-8.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#ifndef fec_para_h
#define fec_para_h


typedef struct {
    int k, x, s, h, l, p;
}fec_para_t;

void _fecParaInit(fec_para_t* para, int k);

#endif

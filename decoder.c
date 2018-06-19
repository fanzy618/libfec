//
//  decoder.c
//  fec
//
//  Created by fanzy on 14-9-11.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "fec.h"
#include "decoder.h"

//5.5.2.1
static void fecDecoderInit(fec* f)
{
    fec_para_t *p = &f->p;
    fec_decoder_t *d = &f->d;
    
    d->m = p->s + p->h + f->cnt;
    d->c = malloc(p->l * sizeof(int));
    for (int i=0; i<p->l; i++) {
        d->c[i] = i;
    }
    
    d->d = malloc(d->m * sizeof(int));
    for (int i = 0; i < d->m; i++) {
        d->d[i] = i;
    }
    
    d->data = malloc(d->m * sizeof(void*));
    d->buff = malloc(d->m * f->blockSize);
    memset(d->buff, 0, d->m * f->blockSize);
    
    for (int i=0; i < d->m; i++) {
        d->data[i] = d->buff + i * f->blockSize;
    }
    
    int idx = -1;
    for (int i = p->s + p->h; i < d->m; i++) {
        for (idx = idx+1; (f->data[idx].flag & FEC_FLAG_DATA) == 0; idx++) {}
        memcpy(d->data[i], f->data[idx].d, f->blockSize);
    }
//    int **q = (int**)d->data;
//    printf("dec data: ");
//    for (int i = 0; i < d->m; i++) {
//        printf(" %d", q[i][0]);
//    }
//    printf("\n");
}

// r1 = r1 ^ r2
static void xorRow(fec *f, int r1, int r2)
{
    fec_matrix_t *m = &f->m;
    fec_decoder_t *d = &f->d;
    
    for (int i=0; i < m->col; i++) {
        m->m[r1][i] ^= m->m[r2][i];
    }
    
    for (int i = 0; i < f->blockSize; i++) {
        d->data[d->d[r1]][i] ^= d->data[d->d[r2]][i];
    }
}

static void swapRow(fec *f, int r1, int r2)
{
    fec_matrix_t *m = &f->m;
    fec_decoder_t *d = &f->d;
    
    uint8_t* p = m->m[r1];
    m->m[r1] = m->m[r2];
    m->m[r2] = p;
    
    int q = d->d[r1];
    d->d[r1] = d->d[r2];
    d->d[r2] = q;
}

static void swapCol(fec *f, int c1, int c2)
{
    fec_matrix_t *m = &f->m;
    fec_decoder_t *d = &f->d;
    
    for (int i = 0; i < m->row; i++) {
        uint8_t p = m->m[i][c1];
        m->m[i][c1] = m->m[i][c2];
        m->m[i][c2] = p;
    }
    
    int q = d->c[c1];
    d->c[c1] = d->c[c2];
    d->c[c2] = q;
}

static void graphAdd(fec_graph_t *g, int row, int col1, int col2)
{
    fec_comp_t* comp = NULL;
    if (g->col[col1] == NULL) {
        if (g->col[col2] == NULL) {
            comp = malloc(sizeof(fec_comp_t));
            memset(comp, 0, sizeof(fec_comp_t));
            g->col[col1] = comp;
            g->col[col2] = comp;
            if (g->header == NULL) {
                g->header = comp;
            } else {
                fec_comp_t* p = g->header;
                for (; p->next != NULL; p = p->next) {}
                p->next = comp;
            }
        } else {
            g->col[col1] = g->col[col2];
            comp = g->col[col2];
        }
    } else {
        if (g->col[col2] == NULL) {
            g->col[col2] = g->col[col1];
        } else {
            if (g->col[col1] != g->col[col2]) {
                fec_comp_t *p = g->col[col2];
                g->col[col1]->cnt += p->cnt;
                
                fec_node_t *q = g->col[col1]->nodes;
                for (; q->next; q = q->next) {}
                q->next = p->nodes;
                p->nodes = NULL;
                g->col[col2] = g->col[col1];
                for (int i = 0; i < sizeof(g->col) / sizeof(void*); i++) {
                    if (g->col[i] == p) {
                        g->col[i] = NULL;
                    }
                }
            }
        }
        comp = g->col[col1];
    }
    
    assert(comp != NULL);
    
    fec_node_t* node = malloc(sizeof(fec_node_t));
    assert(node != NULL);
    
    memset(node, 0, sizeof(fec_node_t));
    node->row = row;
    if (comp->nodes == NULL) {
        comp->nodes = node;
    } else {
        fec_node_t* p = comp->nodes;
        for (; p->next; p = p->next) {}
        p->next = node;
    }
    comp->cnt++;
}

static int graphGetRow(fec_graph_t* g)
{
    fec_comp_t* comp = g->header;
    for (fec_comp_t* p = g->header; p ; p = p->next) {
//        printf("comp size =%d\n", p->cnt);
        if (p->cnt > comp->cnt) {
            comp = p;
        }
    }
    return comp->nodes->row;
}

static void graphFree(fec_graph_t *g)
{
    for (fec_comp_t *p = g->header; p;) {
        for (fec_node_t* q = p->nodes; q;) {
            fec_node_t* m = q->next;
            free(q);
            q = m;
        }
        fec_comp_t *n = p->next;
        free(p);
        p = n;
    }
}

static int findRowMin(fec_matrix_t *m, int start, int end, int *row)
{
    //FIXME
    fec_graph_t graph;
    memset(&graph, 0, sizeof(fec_graph_t));
    int col1, col2;
    int cnt = 0x7FFFFFFF;
    for (int i = start; i < m->row; i++) {
        int c = 0;
        // count ones between start and end
        for (int j = start; j < end; j++) {
            c += m->m[i][j];
            if (m->m[i][j] == 1) {
                if (c == 1) {
                    col1 = j;
                } else if (c == 2) {
                    col2 = j;
                }
            }
        }
        if (c == 2) {
            graphAdd(&graph, i, col1, col2);
        }
        if (c > 0 && cnt > c) {
            cnt = c;
            *row = i;
        }
    }
    if (cnt == 1 << 31) {
        cnt = 0;
    } else if (cnt == 2) {
        *row = graphGetRow(&graph);
    }
    
    graphFree(&graph);
    return cnt;
}

static void reorder(fec *f, int row, int start, int end, int r)
{
    fec_matrix_t *m = &f->m;
    if (m->m[row][start] == 0) {
        for (int i = start; i < end; i++) {
            if (m->m[row][i] == 1) {
                swapCol(f, start, i);
                start = i+1;
                break;
            }
        }
    } else {
        start++;
    }
    r--;
    
    // Move the remaining r-1 ones appear in the last columns of V.
    while (r > 0 ) {
        int i, j;
        for (i = start; i < end; i++) {
            if (m->m[row][i] == 1) {
                break;
            }
        }
        for (j = end - 1; j > i; j--) {
            if (m->m[row][j] == 1) {
                r--;
            } else {
                break;
            }
        }
        if (r == 0) {
            break;
        }
        swapCol(f, i, j);
        start = i;
        end = j;
        r--;
    }
}

// 5.5.2.2
static int decodePhaseOne(fec *f)
{
    fec_para_t *p = &f->p;
    int i = 0, u = 0;
    while (i + u < p->l) {
//        printf("i=%d, u=%d.\n", i, u);
//        _fecMatrixPrint("first", &f->m);
        int row = 0;
        int cnt = findRowMin(&f->m, i, p->l - u, &row);
        if (cnt == 0) {
            //decoding failed.
            return -1;
        }
        if (i != row) {
            swapRow(f, i, row);
        }
//        printf("Swap r1=%d, r2=%d.\n", i, row);
//        _fecMatrixPrint("swap", &f->m);
        reorder(f, i, i, p->l - u, cnt);
//        _fecMatrixPrint("After reorder", &f->m);
        for (int j = i + 1; j < f->m.row; j++) {
            if (f->m.m[j][i] == 1) {
                xorRow(f, j, i);
            }
        }
        
        i++;
        u += cnt - 1;
    }
    f->d.i = i;
    f->d.u = u;
//    printf("i=%d, u=%d.\n", i, u);
//    _fecMatrixPrint("finally", &f->m);
    return 0;
}

//5.5.2.3
static int decodePhaseTwo(fec *f)
{
    fec_para_t *p = &f->p;
    fec_matrix_t *m = &f->m;
    int i = f->d.i;

    for (int j = i; j < p->l; j++) {
        if (m->m[j][j] == 0) {
            int row = j+1;
            for (; row < m->row; row++) {
                if (m->m[row][j] == 1) {
                    swapRow(f, j, row);
                    break;
                }
            }
            if (row == m->row) {
                // Decode failed.
                return -1;
            }
            
        }
        //FIXME
        for (int row = 0; row < m->row; row++) {
            if (row == j) {
                continue;
            }
            if (m->m[row][j] == 1) {
                xorRow(f, row, j);
            }
        }
    }
    return 0;
}

static void reorderC(fec_decoder_t *d, int l)
{
    void** buff = malloc(l * sizeof(void*));
    
    for (int i = 0; i < l; i++) {
        buff[d->c[i]] = d->data[d->d[i]];
    }
    memcpy(d->data, buff, l * sizeof(void*));
    free(buff);
}

// C is the intermediate symbols.
int _fecGetSymbols(void *p)
{
    fec* f = (fec*)p;
    fecDecoderInit(f);
//    int** C = (int**)f->d.data;
//    for (int j = 0; j < f->d.m; j++) {
//        printf(" %d", C[j][0]);
//    }
//    printf("\n");
    if (decodePhaseOne(f) == -1) {
        return -1;
    }
//    _fecMatrixPrint("Phase one completed!", &f->m);
    if (decodePhaseTwo(f) == -1) {
        return -1;
    }
//    _fecMatrixPrint("Phase two completed!", &f->m);
    reorderC(&f->d, f->p.l);
    
    //_fecMatrixPrint("finished.", &f->m);
    return 0;
}

void _fecDecoderClose(fec_decoder_t *d)
{
    free(d->d);
    free(d->c);
    free(d->data);
    free(d->buff);

}


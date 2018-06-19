//
//  para.c
//  fec
//
//  Created by fanzy on 14-9-8.
//  Copyright (c) 2014年 fanzy. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include "fec.h"
#include "para.h"

static int combinatorics(int n, int m) {
    assert(m > 0);
    assert(m <= n);
    int x = 1, y = 1;
    for (int i = m + 1; i <= n; i++) {
        x *= i;
    }
    for (int i = 2; i <= n - m; i++) {
        y *= i;
    }
    return x / y;
}

static int findH(int n) {
    int h;
    for (h = 2; combinatorics(h, (int)ceilf(h/2.0)) < n; h++) {}
    return h;
}

static bool isPrime(int n) {
    assert(n > 1);
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

static int nextPrime(int n) {
    if ((n & 1) == 0) {
        n++;
    }
    for (; !isPrime(n); n+=2) {}
    return n;
}

static int findX(int k)
{
    int x;
    for (x=4; x * (x-1) < 2 * k; x++) {}
    return x;
}

void _fecParaInit(fec_para_t* para, int k)
{
    para->k = k;
    para->x = findX(para->k);
    para->s = nextPrime((int)(ceilf(0.01 * para->k)) + para->x);
    para->h = findH(para->k + para->s);
    para->l = para->k + para->s + para->h;
    para->p = nextPrime(para->l);
}

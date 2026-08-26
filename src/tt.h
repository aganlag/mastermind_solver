#pragma once
#include "logic.h"
#include <stdlib.h>


#ifdef DIAGNOSTICS
    #include <stdio.h>
    #include "diagnostics.h"
#endif

typedef struct {
    uint64_t hash;
    code_t   code;
} tt_entry_t;

#ifndef TT_TOTAL_ENTRIES
    #define TT_TOTAL_ENTRIES 20000
#endif

_Atomic(tt_entry_t)* TT;
uint64_t*            zobrist;


void init_zobrist_arr()
{
    int total_codes = ipow(ALLOWED_DIGITS, CODE_LEN);
    zobrist         = malloc(sizeof(uint64_t) * total_codes);
    for (int c1 = 0; c1 < total_codes; c1++) {
        zobrist[c1] = ((uint64_t) rand() << 32) | rand();
    }
}

static inline uint64_t mix64(uint64_t x)
{
    x += 0x9e3779b97f4a7c15ULL;

    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;

    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;

    x ^= x >> 31;

    return x;
}


static inline void init_TT()
{
    // TT = calloc(TT_TOTAL_ENTRIES, sizeof(tt_entry_t));
    TT = calloc(TT_TOTAL_ENTRIES, sizeof(*TT));
}


#pragma once
#include "logic.h"
#include <stdlib.h>

#if (ALLOWED_DIGITS <= 9) && (CODE_LEN <= 4)
    #define PEGS_LOOKUP_ON
    #pragma message "Using pegs lookup"
#endif


pegs_status_t* PEGS_LOOKUP;


// Allocs and populates the global PEGS_LOOKUP
static inline void init_pegs_lookup()
{
    // At the cost of a more elaborate indexing scheme we could half the size of this array leveraging the simmetry
    // pegs(c1, c2) == pegs(c2, c1).
    //
    // Caching doesnt save time for all settings, in some cases set_pegs() is still faster, but for normal
    // mastermind this **should** be better.

    int total_codes = ipow(ALLOWED_DIGITS, CODE_LEN);
    PEGS_LOOKUP     = malloc(sizeof(pegs_status_t) * total_codes * total_codes);
#pragma omp parallel for
    for (int c1 = 0; c1 < total_codes; c1++) {
        for (int c2 = 0; c2 < total_codes; c2++) {
            PEGS_LOOKUP[c1 * total_codes + c2] = set_pegs(c1, c2);
        }
    }
}

#ifdef PEGS_LOOKUP_ON
    #define GET_PEGS(c1, c2) PEGS_LOOKUP[(c1) + (c2) * (int) ipow(ALLOWED_DIGITS, CODE_LEN)]
#else
    #define GET_PEGS(c1, c2) set_pegs((c1), (c2))
#endif
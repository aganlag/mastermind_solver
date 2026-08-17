#pragma once

#include <stdint.h>
#include "logic.h"
#include <stdio.h>

#define PRINT(X) \
    _Generic((X), \
      int: printf("%ld", (int64_t) X), \
      int8_t: printf("%ld", (int64_t) X), \
      int64_t: printf("%ld", X), \
      float: printf("%.4f", X), \
      double: printf("%.4f", (float) X), \
      uint64_t: printf("%lu", X), \
      uint16_t: printf("%lu", (uint64_t) X), \
      char: printf("%c", X))

#define PRINT_VAR(X) \
\
    printf(#X ": "); \
    PRINT(X); \
    printf(" \n");


#define PRINT_ARR(X) \
    ({ \
        printf(#X ": [ "); \
        for (size_t i = 0; i < (sizeof(X) / sizeof(X[0])); i++) { \
            PRINT(X[i]); \
            printf(" "); \
        } \
        printf("]\n"); \
    })


// slightly painful since they need to be thread safe and all the execution time related stuff is tricky.
typedef struct {


    // TT DIAGNOSTICS
    uint64_t TT_hits;
    uint64_t total_solver_call;
    uint64_t TT_collisions;

    // SEARCH STUFF
    uint64_t total_codes_checked;
    uint64_t cumultaive_candidates_pop[MAX_TRIES];


} diagonstics_t;

diagonstics_t DIAGNOSTICS_STATS = { 0 };

#if TT_TOTAL_ENTRIES > 0
    #include "tt.h"

static inline void print_tt_diagnostics()
{
    printf("Allocated %d slots, ~%.2f Mib\n", TT_TOTAL_ENTRIES,
           (float) TT_TOTAL_ENTRIES * (float) sizeof(tt_entry_t) / 1048576.0);
    // clang-format off
    printf("#################################\n"
           "TT Hits            : %lu\n"
           "TT Misses          : %lu\n"
           "TT Collisions      : %lu\n"
           "Total codes checked: %lu\n"
           "################################\n",
            DIAGNOSTICS_STATS.TT_hits,
            DIAGNOSTICS_STATS.total_solver_call - DIAGNOSTICS_STATS.TT_hits,
            DIAGNOSTICS_STATS.TT_collisions,
            DIAGNOSTICS_STATS.total_codes_checked
        );

    // clang-format on
}
#else
static inline void print_tt_diagnostics()
{
    printf("Allocated 0 slots, ~0.0 Mib\n");
    // clang-format off
    printf("#################################\n"
           "TT Hits            : %lu\n"
           "TT Misses          : %lu\n"
           "TT Collisions      : %lu\n"
           "Total codes checked: %lu\n"
           "################################\n",
            DIAGNOSTICS_STATS.TT_hits,
            DIAGNOSTICS_STATS.total_solver_call - DIAGNOSTICS_STATS.TT_hits,
            DIAGNOSTICS_STATS.TT_collisions,
            DIAGNOSTICS_STATS.total_codes_checked
        );

    // clang-format on
}
    #
#endif
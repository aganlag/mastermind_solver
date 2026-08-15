#pragma once
#include "logic.h"
#include "pegslookup.h"
#include "tt.h"
#include <math.h>
#include <stdbool.h>

#ifdef _OPENMP
    #include <stdatomic.h>
    #include <omp.h>
#endif

#define NO_HISTORY -1

typedef struct {
    code_t* arr;
    int     len;
} candidates_arr_t;

static inline candidates_arr_t init_candidates_array()
{
    candidates_arr_t c = { };
    c.len              = ipow(ALLOWED_DIGITS, CODE_LEN);
    c.arr              = malloc(sizeof(code_t) * c.len);
    for (int code = 0; code < c.len; code++) {
        c.arr[code] = code;
    }
    return c;
}

static inline bool compare_pegs(pegs_status_t p1, pegs_status_t p2)
{
    return (p1.correct == p2.correct) && (p1.misplaced == p2.misplaced);
}

#ifdef PEGS_LOOKUP_ON
    #define GET_PEGS(c1, c2) PEGS_LOOKUP[(c1) + (c2) * (int) ipow(ALLOWED_DIGITS, CODE_LEN)]
#else
    #define GET_PEGS(c1, c2) set_pegs((c1), (c2))
#endif


static inline code_t solver(candidates_arr_t* c, pegs_status_t last_peg_status, code_t last_guessed_code)
{

    uint64_t hash        = 0;
    int      total_codes = ipow(ALLOWED_DIGITS, CODE_LEN);

    tt_entry_plain_t curr_entry;


    // filtering stage is skipped if no codes are yet guessed
    if (last_peg_status.correct != NO_HISTORY) {

        for (int i = 0; i < c->len; i++) {

            if (!compare_pegs(last_peg_status, GET_PEGS(c->arr[i], last_guessed_code))) {
                // The idea is to always keep the array sorted like this:
                // alive candidates [0       ... arr.len - 1    ]
                // dead  candidates [arr.len ... total_codes - 1]

                code_t tmp     = c->arr[i];
                c->arr[i--]    = c->arr[--c->len];
                c->arr[c->len] = tmp;

            } else {
                // we incrementally update the hash of this state if the code is in the alive candidate set
                hash ^= zobrist[c->arr[i]];
            }
        }
#ifdef _OPENMP
        curr_entry = atomic_load(&TT[hash % TT_TOTAL_ENTRIES]);
#else
        curr_entry = TT[hash % TT_TOTAL_ENTRIES];
#endif
        if (curr_entry.hash == hash) {

            return curr_entry.code;
        }
    }


    m_float_t best_score = 666666666;  // big number
    code_t    best_guess = 0;

    for (int i = 0; i < total_codes; i++) {

        m_float_t curr_score                          = 0;
        int       buckets[CODE_LEN + 1][CODE_LEN + 1] = { 0 };

        for (int j = 0; j < c->len; j++) {
            pegs_status_t bucket_i = GET_PEGS(c->arr[i], c->arr[j]);
            buckets[bucket_i.correct][bucket_i.misplaced] += 1;
        }

        for (int k = 0; k < CODE_LEN + 1; k++) {
            for (int h = 0; h < CODE_LEN + 1; h++) {
                if (buckets[k][h] == 0) {
                    continue;
                }

                // The original formula was sum log2(n) * (n / N) but we can avoid the multiplication by 1/N and
                // keep the same ordering, sum log2(n) * n, this makes the formula somewhat less intuitive.
                curr_score += log2((m_float_t) buckets[k][h]) * (m_float_t) buckets[k][h];

                // Same here
                // curr_score += (FLOAT) buckets[k][h] * (FLOAT) buckets[k][h];
            }
        }


        // Without the additional conditions the solver always prefers the smallest index, so a different ordering
        // means a different policy (in case of ties). To avoid inconsistencies, it's better to make the
        // tie-breaking policy fully explicit.
        //
        // This allows us to just reset the array by changing its len while still keeping the tie resolution totally
        // independent from other runs.
        //
        // This ugly looking codition `i < c->len` is rather important, it makes so that in case of a tie an alive
        // candidate is preferred. Having this propriety helps us in one particular occasion; When len == 1 all
        // scores are 0, because log2(1) = 0, so if the solver were not to choose an alive candidate it could then
        // enter a loop. We could solve this by making the len == 1 case explicit above, but having this condition
        // here makes it so in ties alive candidates are always preferred over dead ones thus giving always a chance
        // to the solver to finish the game early.


        if (curr_score < best_score || (curr_score == best_score && c->arr[i] < best_guess && i < c->len)) {
            best_score = curr_score;
            best_guess = c->arr[i];
        }
    }

    tt_entry_plain_t new_entry = { .hash = hash, .code = best_guess };

    if (unlikely(last_peg_status.correct != NO_HISTORY)) {
#ifdef _OPENMP
        atomic_store(&TT[hash % TT_TOTAL_ENTRIES], new_entry);
#else
        TT[hash % TT_TOTAL_ENTRIES] = new_entry;
#endif
    }
    return best_guess;
}

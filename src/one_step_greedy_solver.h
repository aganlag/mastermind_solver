#pragma once
#include "logic.h"
#include "pegslookup.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>


#include "diagnostics.h"
#include "tt.h"


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
    return p1.bits == p2.bits;
}

static inline float approx_log2(float a)
{
    typedef union {
        float    f;
        uint32_t b;
    } bits_t;

    bits_t x = { .f = a };

    int32_t exp = ((x.b >> 23) & 0xFF) - 127;
    bits_t  m   = { .b = (x.b & 0x007FFFFF) | (127u << 23) };

    // quadratic approximation
    // float logm = -0.3369f * m.f * m.f + 1.995f * m.f - 1.649f;
    // float logm = m.f * (-0.3369f * m.f + 1.995f) - 1.649f;
    // linear approximation
    // float logm = 0.9843f * m.f - 0.9191f;
    float logm = m.f - 1;

    return exp + logm;
}

// FILTER CANDIDATES AND RETURN HASH
static inline uint64_t filter_candidates(candidates_arr_t* c, pegs_status_t last_peg_status, code_t last_guessed_code)
{
    int total_codes = ipow(ALLOWED_DIGITS, CODE_LEN);

    uint64_t hash = 0;
    for (int i = 0; i < c->len; i++) {

        if (last_peg_status.bits != GET_PEGS(c->arr[i], last_guessed_code, total_codes).bits) {
            // The idea is to always keep the array sorted like this:
            // alive candidates [0       ... arr.len     - 1]
            // dead  candidates [arr.len ... total_codes - 1]

            code_t tmp     = c->arr[i];
            c->arr[i--]    = c->arr[--c->len];
            c->arr[c->len] = tmp;
            continue;
        }
        hash ^= zobrist[c->arr[i]];
    }
    return hash;
}
int cnt = 0;


static inline code_t solver(candidates_arr_t* c)
{
    // ASSUMES THE ARRAY C IS ALREADY FILTERED

    int total_codes = ipow(ALLOWED_DIGITS, CODE_LEN);


    float  best_score = INFINITY;  // big number
    code_t best_guess = 0;

    if (unlikely(c->len == 1)) {
        return c->arr[0];
    } else if (unlikely(c->len == 2)) {
        return c->arr[0] > c->arr[1] ? c->arr[1] : c->arr[0];
    }

    int tot_b = ((CODE_LEN + 1) * (CODE_LEN + 1) + (CODE_LEN + 1)) / 2 - 1;

    for (int i = 0; i < total_codes; i++) {

        uint16_t buckets[CODE_LEN + 1][CODE_LEN + 1] = { 0 };
        float    curr_score                          = 0;


        for (int j = 0; j < c->len; j++) {

            // under this threshold we can try to cut, i dont know what this threshold should be...
            // if (unlikely(c->len <= tot_b)) {
            //    int r = (c->len - (j + 1));
            //    // optimistic estimate
            //    float estimate = ((float) r / (float) tot_b) * approx_log2((float) r / (float) tot_b);
            //
            //    if (estimate + curr_score > best_score) {
            //        curr_score += estimate;
            //        break;
            //    }
            //}

            pegs_status_t bucket_i = GET_PEGS(c->arr[i], c->arr[j], total_codes);
            int           visits   = ++buckets[bucket_i.correct][bucket_i.missplaced];

            // no need to check for 0 because approx_log_2 returns -127 on log2(0) and it gets cancelled out by * 0
            // float prev_ent = approx_log2(visits - 1) * (visits - 1);
            // float new_ent  = approx_log2(visits) * visits;

            float delta_approx = approx_log2(visits * 0.5) + 1 / 0.69314718056;
            curr_score += delta_approx;


            if (curr_score > best_score) {
                break;
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

            // we found a score we cant improve on, rather cheap check and potentially extremely beneficial.
            if (unlikely(best_score <= 0)) {
                return best_guess;
            }
        }
    }
    return best_guess;
}

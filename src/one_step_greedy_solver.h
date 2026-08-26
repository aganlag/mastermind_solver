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
    return (p1.correct == p2.correct) && (p1.misplaced == p2.misplaced);
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

        if (!compare_pegs(last_peg_status, GET_PEGS(c->arr[i], last_guessed_code, total_codes))) {
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


    m_float_t best_score = 666666666666;  // big number
    code_t    best_guess = 0;

    if (c->len == 1) {
        return c->arr[0];
    }


    for (int i = 0; i < total_codes; i++) {

        m_float_t curr_score = 0;

        uint16_t buckets[CODE_LEN + 1][CODE_LEN + 1] = { 0 };

        float_t bound = c->len * approx_log2(1.f);

        for (int j = 0; j < c->len; j++) {

            // int r = (c->len - (j + 1));
            //
            ////  loose bound
            //// integer bound not faster coz its expensive
            // int   q        = r / TOT_B;
            // int   rem      = r % TOT_B;
            // float estimate = (TOT_B - rem) * q * approx_log2(q) + rem * (q + 1) * approx_log2(q + 1);
            //
            //// less tight but faster
            //// float estimate = ((float_t) r / (float_t) TOT_B) * approx_log2((float_t) r / (float_t) TOT_B);
            //
            // if (unlikely(estimate + curr_score > best_score)) {
            //    curr_score += estimate;
            //    break;
            //}

            pegs_status_t bucket_i = GET_PEGS(c->arr[i], c->arr[j], total_codes);
            int           visits   = ++buckets[bucket_i.correct][bucket_i.misplaced];
            float         prev_ent = (visits - 1 > 0) * approx_log2(visits - 1) * (visits - 1);
            float         new_ent  = approx_log2(visits) * visits;
            curr_score += new_ent - prev_ent;

            if (curr_score <= bound) { }

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
        }
    }
    return best_guess;
}

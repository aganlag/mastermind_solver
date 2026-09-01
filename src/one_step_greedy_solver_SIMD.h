#pragma once
#include "logic.h"
#include "pegslookup.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>


#include "diagnostics.h"
#include "tt.h"
#include <immintrin.h>

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

static inline __m256 approx_log_simd(__m256 a)
{

    const __m256i exponent_mask = _mm256_set1_epi32(0xFF);
    const __m256i mantissa_mask = _mm256_set1_epi32(0x007FFFFF);
    const __m256i exponent_bias = _mm256_set1_epi32(127);
    const __m256i one_bits      = _mm256_set1_epi32(127u << 23);

    // EXPONENT
    __m256i exp_b = _mm256_srli_epi32(_mm256_castps_si256(a), 23);
    exp_b         = _mm256_and_si256(exp_b, exponent_mask);
    exp_b         = _mm256_sub_epi32(exp_b, exponent_bias);
    __m256 exp_f  = _mm256_cvtepi32_ps(exp_b);

    // MANTISSA
    __m256i m_b = _mm256_and_si256(_mm256_castps_si256(a), mantissa_mask);
    m_b         = _mm256_or_si256(m_b, one_bits);
    __m256 m_f  = _mm256_sub_ps(_mm256_castsi256_ps(m_b), _mm256_set1_ps(1.0f));

    return _mm256_add_ps(m_f, exp_f);
}

static inline float approx_log2(float a)
{
    typedef union {
        float    f;
        uint32_t b;
    } bits_t;

    bits_t x = { .f = a };

    int32_t exp  = ((x.b >> 23) & 0xFF) - 127;
    bits_t  m    = { .b = (x.b & 0x007FFFFF) | (127u << 23) };
    float   logm = m.f - 1;

    return exp + logm;
}

typedef union {
    float  arr[8];
    __m256 data;
} simd_block;


static inline code_t solver(candidates_arr_t* c)
{

    int total_codes = ipow(ALLOWED_DIGITS, CODE_LEN);


    float  best_score = 666666666.0f;  // big number
    code_t best_guess = 0;

    if (unlikely(c->len == 1)) {
        return c->arr[0];
    } else if (unlikely(c->len == 2)) {
        return c->arr[0] > c->arr[1] ? c->arr[1] : c->arr[0];
    }

    for (int i = 0; i < total_codes; i++) {

        uint16_t buckets[CODE_LEN + 1][CODE_LEN + 1] = { 0 };
        float    curr_score                          = 0;

        int j = 0;
        for (; j + 8 <= c->len; j += 8) {

            simd_block delta = { };
            for (int k = 0; k < 8; k++) {
                // possible win with GET_PEGS returning vector
                pegs_status_t bucket_i = GET_PEGS(c->arr[i], c->arr[j + k], total_codes);
                delta.arr[k]           = ++buckets[bucket_i.correct][bucket_i.missplaced];
            }

            // no need to check for 0 because approx_log_2 returns -127 on log2(0) and it gets cancelled out by * 0
            // float prev_ent = approx_log2(visits - 1) * (visits - 1);
            // float new_ent  = approx_log2(visits) * visits;
            // the idea is that xlog2(x) - (x - 1)log2(x - 1) = log2(x - 1) + x(log2(x / (x - 1)))
            // as x -> infinity: x(log2(x / (x - 1))) -> 1 / ln(2)
            // so for big x: log2(x - 1) + 1 / ln(2) is a good approximation
            // we fix it at 0.5 instead of 1.0 to get a nicer approximation earlier, 100% there is a sweaty egghead way
            // to justify the 0.5

            const __m256 one_half     = _mm256_set1_ps(0.5f);
            const __m256 one_over_ln2 = _mm256_set1_ps(1.44269504089f);
            delta.data = _mm256_add_ps(approx_log_simd(_mm256_sub_ps(delta.data, one_half)), one_over_ln2);

            // horizontal add
            __m128 lo  = _mm256_castps256_ps128(delta.data);
            __m128 hi  = _mm256_extractf128_ps(delta.data, 1);
            __m128 sum = _mm_add_ps(lo, hi);
            sum        = _mm_hadd_ps(sum, sum);
            sum        = _mm_hadd_ps(sum, sum);
            curr_score += _mm_cvtss_f32(sum);

            if (curr_score > best_score) {
                j = c->len;
                break;
            }
        }
        // SCALAR TAIL
        for (; j < c->len; j++) {

            pegs_status_t bucket_i = GET_PEGS(c->arr[i], c->arr[j], total_codes);
            int           visits   = ++buckets[bucket_i.correct][bucket_i.missplaced];

            float delta_approx = approx_log2(visits - 0.5) + 1 / 0.69314718056;
            curr_score += delta_approx;


            if (curr_score > best_score) {
                break;
            }
        }


        if (curr_score < best_score || (curr_score == best_score && c->arr[i] < best_guess && i < c->len)) {
            best_score = curr_score;
            best_guess = c->arr[i];

            if (unlikely(best_score <= 0)) {
                return best_guess;
            }
        }
    }
    return best_guess;
}
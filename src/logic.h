#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_TRIES   20
#define unlikely(x) __builtin_expect(!!(x), 0)
#define likely(x)   __builtin_expect(!!(x), 1)


typedef double   m_float_t;
typedef uint64_t code_t;


uint64_t* PACK_CODES;

#ifndef ALLOWED_DIGITS
    #define ALLOWED_DIGITS 6
#endif
#ifndef CODE_LEN
    #define CODE_LEN 4
#endif

#define TOT_B ((CODE_LEN + 1) * (CODE_LEN + 1) + (CODE_LEN + 1)) / 2 - 1

// arguably very bad
#define SCAN_DIGIT_MOD_DIV(X) \
    ({ \
        int digit = (X) % ALLOWED_DIGITS; \
        (X) /= ALLOWED_DIGITS; \
        digit; \
    })

#define SCAN_DIGIT_AND_SHIFT(X) \
    ({ \
        int digit = (X) & 0b1111; \
        (X) >>= 4; \
        digit; \
    })


typedef struct {
    int8_t correct;
    int8_t misplaced;
} pegs_status_t;


static inline uint64_t pack_code(uint64_t c)
{
    uint64_t packed = 0;
    for (int i = 0; i < CODE_LEN; i++) {
        uint64_t d1 = SCAN_DIGIT_MOD_DIV(c);
        packed |= d1 << (i * 4);
    }
    return packed;
}

static inline pegs_status_t set_pegs(code_t c1, code_t c2)
{
    pegs_status_t pegs = { 0 };

    int_fast8_t counter[ALLOWED_DIGITS] = { 0 };


    // uint64_t p_c1 = PACK_CODES[c1];
    // uint64_t p_c2 = PACK_CODES[c2];
    for (int i = 0; i < CODE_LEN; i++) {

        int d1 = SCAN_DIGIT_MOD_DIV(c1);
        int d2 = SCAN_DIGIT_MOD_DIV(c2);

        // int d1 = SCAN_DIGIT_AND_SHIFT(p_c1);
        // int d2 = SCAN_DIGIT_AND_SHIFT(p_c2);


        if (unlikely(d1 == d2)) {
            pegs.correct++;
        } else {

            if (counter[d1]++ < 0) {
                pegs.misplaced++;
            }
            if (counter[d2]-- > 0) {
                pegs.misplaced++;
            }
        }
    }
    return pegs;
}


static inline int ipow(int a, uint64_t b)
{
    int result = a;
    for (size_t i = 0; i < b - 1; i++) {
        result *= a;
    }
    return result;
}
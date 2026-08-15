#pragma once
#include <stddef.h>
#include <stdint.h>


#define unlikely(x) __builtin_expect(!!(x), 0)


typedef double   m_float_t;
typedef uint16_t code_t;


#ifndef ALLOWED_DIGITS
    #define ALLOWED_DIGITS 6
#endif
#ifndef CODE_LEN
    #define CODE_LEN 4
#endif


#define SCAN_DIGIT(X) \
    ({ \
        int digit = (X) % ALLOWED_DIGITS; \
        (X) /= ALLOWED_DIGITS; \
        digit; \
    })

typedef struct {
    int8_t correct;
    int8_t misplaced;
} pegs_status_t;

static inline pegs_status_t set_pegs(code_t c1, code_t c2)
{
    pegs_status_t pegs = { 0 };

    int_fast8_t counter[ALLOWED_DIGITS] = { 0 };

    for (int i = 0; i < CODE_LEN; i++) {
        int d1 = SCAN_DIGIT(c1);
        int d2 = SCAN_DIGIT(c2);

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
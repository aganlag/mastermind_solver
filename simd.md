
# Simdable stuff:
- [x] SOLVER
- [ ] SET_PEGS
- [ ] FILTER AND HASH

#

# Comptime switch
```c
#if defined(__AVX512F__)
    #define SIMD_SIZE 64
    #define SIMD_AVX512
#elif defined(__AVX2__)
    #define SIMD_SIZE 32
    #define SIMD_AVX
#elif defined( __SSE2__)
    #define SIMD_SIZE 16
    #define SIMD_SSE
#else
    #define NO_SIMD
#endif
```
# set_pegs sketch

```c

#include <stdint.h>

#define SIMD_SIZE 32
#define ALLOWED_DIGITS 16
#define CODE_LEN 4

typedef uint32_t code_t;
typedef uint32_t pegs_t;

typedef code_t v_code_t __attribute__((vector_size(SIMD_SIZE)));
typedef pegs_t v_pegs_t __attribute__((vector_size(SIMD_SIZE)));
typedef int32_t v_count_t __attribute__((vector_size(SIMD_SIZE)));
typedef uint32_t v_digit_t __attribute__((vector_size(SIMD_SIZE)));

extern const v_digit_t digits[ALLOWED_DIGITS];

v_pegs_t set_pegs(v_code_t c1, v_code_t c2){

    v_count_t h1[ALLOWED_DIGITS] = {0};
    v_count_t h2[ALLOWED_DIGITS] = {0};


    v_pegs_t correct = {0}; 
    v_pegs_t missplaced = {0};

    for(int i = 0; i < CODE_LEN; i++){

        v_digit_t d1 = c1 % ALLOWED_DIGITS;
        v_digit_t d2 = c2 % ALLOWED_DIGITS;
        c1 /= ALLOWED_DIGITS;
        c2 /= ALLOWED_DIGITS;

        correct -= (d1 == d2);
        //for(int l = 0; l < SIMD_SIZE / 32; l++){
        //    h1[d1[l]][l] += 1; 
        //    h2[d2[l]][l] += 1; 
        //}

        for(int d = 0; d < ALLOWED_DIGITS; d++){
            h1[d] -= (d1 == d); 
            h2[d] -= (d2 == d);
        }
    }
    for(int i = 0; i < ALLOWED_DIGITS; i++){
        v_count_t lt = (h1[i] < h2[i]);
        v_count_t min  = (lt & h1[i]) | (~lt & h2[i]);
        missplaced += min;
    }

    missplaced -= correct;


    return (correct << 16) + (missplaced & 0xFFFF);

}


```


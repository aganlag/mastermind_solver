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


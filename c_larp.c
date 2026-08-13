#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ALLOWED_DIGITS 6
#define MAX_CODE_LEN   4
#define NO_HISTORY     -1

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef float    FLOAT;
typedef uint16_t INT;

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

typedef struct {
    INT* arr;
    int  len;
} candidates_arr_t;

// if the printed code exceeds MAX_CODE_LEN it will be cut
void print_code(INT code)
{
    char  code_str[64]    = { '\0' };
    char* colors_lookup[] = { "🟥", "🟧", "🟨", "🟩", "🟦", "🟪" };
    int   str_idx         = MAX_CODE_LEN;

    while (str_idx > 0) {
        int digit           = SCAN_DIGIT(code);
        code_str[--str_idx] = digit + 48;
        // printf("%s", colors_lookup[digit]);
    }
    // printf("\n");
    printf("%s\n", code_str);
}

pegs_status_t set_pegs(INT c1, INT c2)
{
    pegs_status_t pegs = { 0 };

    int_fast8_t counter[ALLOWED_DIGITS] = { 0 };

    for (int i = 0; i < MAX_CODE_LEN; i++) {
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

bool static inline compare_pegs(pegs_status_t p1, pegs_status_t p2)
{
    return (p1.correct == p2.correct) && (p1.misplaced == p2.misplaced);
}

candidates_arr_t generate_initial_candidates()
{
    candidates_arr_t c = { };
    c.len              = pow(ALLOWED_DIGITS, MAX_CODE_LEN);
    c.arr              = malloc(sizeof(INT) * c.len);
    for (int code = 0; code < c.len; code++) {
        c.arr[code] = code;
    }
    return c;
}

INT solver(candidates_arr_t* c, pegs_status_t* pegs_lookup, pegs_status_t last_peg_status, INT last_guessed_code)
{
    // filtering stage is skipped if no codes are yet guessed
    int total_codes = pow(ALLOWED_DIGITS, MAX_CODE_LEN);

    for (int i = 0; i < c->len && last_peg_status.correct != NO_HISTORY; i++) {
        if (!compare_pegs(last_peg_status, pegs_lookup[c->arr[i] + last_guessed_code * total_codes])) {

            // The idea is to always keep the array sorted like this:
            // alive candidates [0       ... arr.len - 1    ]
            // dead  candidates [arr.len ... total_codes - 1]

            INT tmp        = c->arr[i];
            c->arr[i--]    = c->arr[--c->len];
            c->arr[c->len] = tmp;
        }
    }

    FLOAT best_score = INFINITY;
    INT   best_guess = 0;


    // The way we are mantaining the array, keeping all the alive candidates
    // from 0 to len - 1 and all dead ones from len to total_codes - 1,
    // combined with the fact that we are checking codes in the order of the
    // array gives us a nice propriety: in case of ties an alive candidate is
    // always preferred.
    //
    // This becomes relevant when the len is 1. When len = 1 all scores are 0
    // because log2(1) = 0, so the solver just chooses the first code it
    // encounters. The solver could then enter a loop if the remaning alive
    // candidate were not preferred.
    //
    // Prefering alive candidates seems also a nicer scoring policy
    // altogether: it gives the solver a chance of ending the game early.
    for (int i = 0; i < total_codes; i++) {

        FLOAT curr_score                                  = 0;
        int   buckets[MAX_CODE_LEN + 1][MAX_CODE_LEN + 1] = { 0 };

        for (int j = 0; j < c->len; j++) {

            pegs_status_t bucket_i = pegs_lookup[c->arr[i] + c->arr[j] * total_codes];
            buckets[bucket_i.correct][bucket_i.misplaced] += 1;
        }

        for (int k = 0; k < MAX_CODE_LEN + 1; k++) {
            for (int h = 0; h < MAX_CODE_LEN + 1; h++) {
                if (buckets[k][h] == 0) {
                    continue;
                }
                // curr_score += log2((FLOAT) buckets[k][h]) * ((FLOAT) buckets[k][h] / (FLOAT) c->len);
                curr_score += (FLOAT) buckets[k][h] * ((FLOAT) buckets[k][h] / (FLOAT) c->len);
            }
        }

        if (curr_score < best_score) {
            best_score = curr_score;
            best_guess = c->arr[i];
        }
    }

    return best_guess;
}

int main()
{
    srand(time(NULL));


    candidates_arr_t starting_candidates = generate_initial_candidates();
    int              starting_len        = starting_candidates.len;
    int              total               = 0;
    int              buckets[10]         = { 0 };


    // At the cost of a more elaborate indexing scheme we could half the size of this array leveraging the simmetry
    // pegs(c1, c2) == pegs(c2, c1).
    // Caching doesnt save time for all settings, in some cases set_pegs() is still faster, but for normal mastermind
    // this is better.
    pegs_status_t* pegs_lookup = malloc(sizeof(pegs_status_t) * starting_len * starting_len);

#pragma omp parallel for
    for (int c1 = 0; c1 < starting_len; c1++) {
        for (int c2 = 0; c2 < starting_len; c2++) {
            pegs_lookup[c1 + c2 * starting_len] = set_pegs(c1, c2);
        }
    }

    // The first guess needs to only be computed once
    // since it should not change across different games with the same settings

    INT first_guess = 9;  // A depth 2 search found this as the code that minimizes average guesses (using the solver)

    free(starting_candidates.arr);

#pragma omp parallel
    {
        // candidates_arr_t candidates = generate_initial_candidates();

#pragma omp for reduction(+ : total)
        for (int c = 0; c < starting_len; c++) {

            // only possible because of how we are filtering the array
            // candidates.len = starting_len;

            // We do this to avoid lekeage
            candidates_arr_t candidates = generate_initial_candidates();

            INT secret = c;

            INT           guess = first_guess;
            pegs_status_t pegs  = { 0 };

            int tries = 0;

            while (true) {
                tries++;
                pegs = set_pegs(guess, secret);
                if (pegs.correct == MAX_CODE_LEN)
                    break;
                guess = solver(&candidates, pegs_lookup, pegs, guess);
            }
#pragma omp atomic update
            buckets[tries]++;
            total += tries;
            free(candidates.arr);
        }
    }


    free(pegs_lookup);
    printf("[");
    for (int i = 0; i < 10; i++) {
        printf(" %d", buckets[i]);
    }
    printf("]");

    printf(" %f\n", (FLOAT) total / (FLOAT) pow(ALLOWED_DIGITS, MAX_CODE_LEN));
    return 0;
}
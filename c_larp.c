#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _OPENMP
    #include <stdatomic.h>
    #include <omp.h>
#endif

#define ALLOWED_DIGITS 15
#define MAX_CODE_LEN   3
#define NO_HISTORY     -1


#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef double   FLOAT;
typedef uint16_t INT;

#define SCAN_DIGIT(X) \
    ({ \
        int digit = (X) % ALLOWED_DIGITS; \
        (X) /= ALLOWED_DIGITS; \
        digit; \
    })

typedef struct {
    INT* arr;
    int  len;
} candidates_arr_t;

// TT SETUP
uint64_t* zobrist;


typedef struct {
    uint64_t hash;
    INT      code;
} tt_entry_plain_t;

#ifdef _OPENMP
typedef _Atomic tt_entry_plain_t tt_entry_t;
#else
typedef tt_entry_plain_t tt_entry_t;
#endif

#define TT_TOTAL_ENTRIES (1000000)
tt_entry_t* TT;


typedef struct {
    int8_t correct;
    int8_t misplaced;
} pegs_status_t;

// extremely arbitrary
#if ALLOWED_DIGITS <= 9 && MAX_CODE_LEN <= 4
    #define PEGS_LOOKUP_ON

pegs_status_t* pegs_lookup;

#endif


// if the printed code exceeds MAX_CODE_LEN it will be cut
// void print_code(INT code)
//{
//    char  code_str[64]    = { '\0' };
//    char* colors_lookup[] = { "🟥", "🟧", "🟨", "🟩", "🟦", "🟪" };
//    int   str_idx         = MAX_CODE_LEN;
//
//    while (str_idx > 0) {
//        int digit           = SCAN_DIGIT(code);
//        code_str[--str_idx] = digit + 48;
//        // printf("%s", colors_lookup[digit]);
//    }
//    // printf("\n");
//    printf("%s\n", code_str);
//}

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

pegs_status_t static inline get_pegs(INT c1, INT c2)
{
#ifdef PEGS_LOOKUP_ON
    return pegs_lookup[c1 + c2 * (int) pow(ALLOWED_DIGITS, MAX_CODE_LEN)];
#else
    return set_pegs(c1, c2);
#endif
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


INT solver(candidates_arr_t* c, pegs_status_t last_peg_status, INT last_guessed_code)
{

    uint64_t hash        = 0;
    int      total_codes = pow(ALLOWED_DIGITS, MAX_CODE_LEN);

    tt_entry_plain_t curr_entry;


    // filtering stage is skipped if no codes are yet guessed
    if (last_peg_status.correct != NO_HISTORY) {

        for (int i = 0; i < c->len; i++) {

            if (!compare_pegs(last_peg_status, get_pegs(c->arr[i], last_guessed_code))) {
                // The idea is to always keep the array sorted like this:
                // alive candidates [0       ... arr.len - 1    ]
                // dead  candidates [arr.len ... total_codes - 1]

                INT tmp        = c->arr[i];
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


    FLOAT best_score = INFINITY;
    INT   best_guess = 0;

    for (int i = 0; i < total_codes; i++) {

        FLOAT curr_score                                  = 0;
        int   buckets[MAX_CODE_LEN + 1][MAX_CODE_LEN + 1] = { 0 };

        for (int j = 0; j < c->len; j++) {
            pegs_status_t bucket_i = get_pegs(c->arr[i], c->arr[j]);
            buckets[bucket_i.correct][bucket_i.misplaced] += 1;
        }

        for (int k = 0; k < MAX_CODE_LEN + 1; k++) {
            for (int h = 0; h < MAX_CODE_LEN + 1; h++) {
                if (buckets[k][h] == 0) {
                    continue;
                }

                // The original formula was sum log2(n) * (n / N) but we can avoid the multiplication by 1/N and
                // keep the same ordering, sum log2(n) * n, this makes the formula somewhat less intuitive.
                curr_score += log2((FLOAT) buckets[k][h]) * (FLOAT) buckets[k][h];

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

int main()
{
    srand(time(NULL));


    TT = calloc(sizeof(tt_entry_t), TT_TOTAL_ENTRIES);


    int starting_len = pow(ALLOWED_DIGITS, MAX_CODE_LEN);
    int total        = 0;
    int buckets[10]  = { 0 };


    zobrist = malloc(sizeof(uint64_t) * starting_len);
    // init zobrist hashing status
    for (int c1 = 0; c1 < starting_len; c1++) {
        zobrist[c1] = ((uint64_t) rand() << 32) | rand();
    }

#ifdef PEGS_LOOKUP_ON

    // At the cost of a more elaborate indexing scheme we could half the size of this array leveraging the simmetry
    // pegs(c1, c2) == pegs(c2, c1).
    //
    // Caching doesnt save time for all settings, in some cases set_pegs() is still faster, but for normal
    // mastermind this **should** be better.
    pegs_lookup = malloc(sizeof(pegs_status_t) * starting_len * starting_len);


    #pragma omp parallel for
    for (int c1 = 0; c1 < starting_len; c1++) {
        for (int c2 = 0; c2 < starting_len; c2++) {
            pegs_lookup[c1 + c2 * starting_len] = set_pegs(c1, c2);
        }
    }
#endif

    // The first guess needs to only be computed once since it should not change across different games with the
    // same settings

#if ALLOWED_DIGITS == 6 && MAX_CODE_LEN == 4

    // An exhaustive search (each possible first guess × each possible secret)
    // using the solver found this as the starting code that minimizes average
    // guesses.

    INT first_guess = 9;
#else
    candidates_arr_t starting_candidates = generate_initial_candidates();
    INT              first_guess         = solver(&starting_candidates, (pegs_status_t){ .correct = NO_HISTORY }, 0);
    free(starting_candidates.arr);
#endif


#pragma omp parallel
    {
        candidates_arr_t candidates = generate_initial_candidates();

#pragma omp for reduction(+ : total)
        for (int c = 0; c < starting_len; c++) {

            // only possible because of how we are filtering the array
            candidates.len = starting_len;

            INT secret = c;

            INT           guess = first_guess;
            pegs_status_t pegs  = { 0 };

            int tries = 0;

            while (true) {
                tries++;
                pegs = set_pegs(guess, secret);
                if (pegs.correct == MAX_CODE_LEN)
                    break;
                guess = solver(&candidates, pegs, guess);
            }
#pragma omp atomic update
            buckets[tries]++;
            total += tries;
            // printf("%d %d %d\n", secret, starting_len, tries);
        }
    }

#ifdef PEGS_LOOKUP_ON
    free(pegs_lookup);
#endif
    // printf("\n");
    for (int i = 1; i < 10; i++) {
        printf("%d ", buckets[i]);
    }
    printf("\n");

    printf("%f\n", (FLOAT) total / (FLOAT) starting_len);
    printf("%d\n", ALLOWED_DIGITS);
    printf("%d\n", MAX_CODE_LEN);

    free(TT);
    return 0;
}
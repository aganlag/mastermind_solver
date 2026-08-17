#include "logic.h"
#include "one_step_greedy_solver.h"

#include <stdint.h>
#include <stdio.h>


#ifdef DIAGNOSTICS
    #include "diagnostics.h"
    #pragma message("Diagnostics ON")
#endif

#ifdef PEGS_LOOKUP_ON
    #include "pegslookup.h"
    #pragma message("Pegs Lookup ON")
#endif

#ifdef _OPENMP
    #pragma message("Multithreading ON")
    #define MULTITHREADING 1
#else
    #define MULTITHREADING 0
#endif

#if TT_TOTAL_ENTRIES > 0
    #include "tt.h"
    #pragma message("TT ON")
#endif

int main()
{
    int starting_len = ipow(ALLOWED_DIGITS, CODE_LEN);
#ifdef PEGS_LOOKUP_ON
    init_pegs_lookup();
#endif

#if TT_TOTAL_ENTRIES > 0
    init_zobrist_arr();
    init_TT();
#endif

    // Run the solver for all these configuration, could be smarter and run (each START X each SECRETS) and pick
    // theti START that gets the lowest average number of moves. As is i just run the computation of the else
    // below.
    code_t cached_first_guesses[20][20] = {
#include "../cached_first_guesses.txt"
    };

    code_t first_guess;

#ifdef SEARCH_FIRST_GUESS
    first_guess = (code_t) -1;
#else
    first_guess = cached_first_guesses[ALLOWED_DIGITS][CODE_LEN];
#endif
    if (first_guess == (code_t) -1) {
        // Find a reasonable first guess
        candidates_arr_t candidates = init_candidates_array();
        first_guess                 = solver(&candidates, (pegs_status_t){ .correct = NO_HISTORY }, 0);
        free(candidates.arr);
    }
// Together with the search_first_guess.fish it fills the array above
#ifdef SEARCH_FIRST_GUESS
    printf("[%d][%d] = %d, \n", ALLOWED_DIGITS, CODE_LEN, first_guess);
    return 0;
#endif
    int total              = 0;
    int buckets[MAX_TRIES] = { 0 };

#pragma omp parallel
    {
        candidates_arr_t candidates = init_candidates_array();

#pragma omp for reduction(+ : total)
        for (int c = 0; c < starting_len; c++) {

            // only possible because of how we are filtering the array
            candidates.len = starting_len;

            code_t secret = c;

            code_t        guess = first_guess;
            pegs_status_t pegs  = { 0 };

            int tries = 0;
            while (true) {
                tries++;
#ifdef DIAGNOSTICS
    #pragma omp atomic update
                DIAGNOSTICS_STATS.cumultaive_candidates_pop[tries] += candidates.len;
#endif
                pegs = set_pegs(guess, secret);
                if (pegs.correct == CODE_LEN)
                    break;
                guess = solver(&candidates, pegs, guess);
            }
#pragma omp atomic update
            buckets[tries]++;
            total += tries;
        }
    }


#pragma omp barrier
    float avg_try = (m_float_t) total / (m_float_t) starting_len;
#ifdef DIAGNOSTICS
    float avg_pop[MAX_TRIES]     = { 0 };
    float avg_entropy[MAX_TRIES] = { 0 };
    float avg_winrate[MAX_TRIES] = { 0 };

    float total_pop = starting_len;
    for (size_t i = 1; i < (sizeof(buckets) / sizeof(buckets[0])); i++) {
        avg_pop[i]     = (float) DIAGNOSTICS_STATS.cumultaive_candidates_pop[i] / total_pop;
        avg_winrate[i] = buckets[i] / total_pop;
        avg_entropy[i] = log2(avg_pop[i]);
        total_pop -= buckets[i];
        if (total_pop <= 0) {
            break;
        }
    }
    PRINT_VAR(ALLOWED_DIGITS)
    PRINT_VAR(CODE_LEN)
    PRINT_ARR(buckets);
    PRINT_ARR(avg_entropy);
    PRINT_ARR(avg_pop);
    PRINT_ARR(avg_winrate);
    PRINT_VAR(MULTITHREADING);
    print_tt_diagnostics();
#endif

#ifdef PEGS_LOOKUP_ON
    free(PEGS_LOOKUP);
#endif

#if TT_TOTAL_ENTRIES > 0
    free(TT);
#endif

    printf("AVG TRY: %f\n\n", avg_try);
    return 0;
}
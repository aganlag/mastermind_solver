#include "logic.h"
#include "one_step_greedy_solver.h"
#include "tt.h"
#include "pegslookup.h"

#include <stdio.h>

#define DEBUG_VAL_L(X)    printf("val: %d line: %d\n", (X), __LINE__);
#define DEBUG_VAL(MSG, X) printf(#MSG ": %d\n", (X));
#define PRINT_MACRO(X)    printf(#X ": %d\n", (X));


int main()
{

    int starting_len = ipow(ALLOWED_DIGITS, CODE_LEN);

#ifdef PEGS_LOOKUP_ON
    init_pegs_lookup();
#endif
    init_zobrist_arr();
    init_TT();


    // Run the solver for all these configuration, could be smarter and run (each START X each SECRETS) and pick theti
    // START that gets the lowest average number of moves. As is i just run the computation of the else below.
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

    int total       = 0;
    int buckets[10] = { 0 };

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
                pegs = set_pegs(guess, secret);
                if (pegs.correct == CODE_LEN)
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
    free(PEGS_LOOKUP);
#endif
    // printf("\n");
    // for (int i = 1; i < 10; i++) {
    //    printf("%d ", buckets[i]);
    //}
    // printf("\n");
    //
    // printf("%f\n", (m_float_t) total / (m_float_t) starting_len);
    // printf("%d\n", ALLOWED_DIGITS);
    // printf("%d\n", CODE_LEN);

    free(TT);
    return 0;
}
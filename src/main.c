#include "logic.h"
#include "one_step_greedy_solver.h"

#include <stdint.h>
#include <stdio.h>
#include "tt.h"

#include "diagnostics.h"

#include <stdatomic.h>
#include <omp.h>
#include <stdlib.h>


#ifdef PEGS_LOOKUP_ON
    #include "pegslookup.h"
    #pragma message("Pegs Lookup ON")
#endif

#ifdef _OPENMP
    #pragma message("Multithreading ON")
#endif


int main()
{
    int starting_len = ipow(ALLOWED_DIGITS, CODE_LEN);


    // PACK_CODES = malloc(starting_len * sizeof(*PACK_CODES));
    // for (int c = 0; c < starting_len; c++) {
    //     PACK_CODES[c] = pack_code(c);
    // }


#ifdef PEGS_LOOKUP_ON
    init_pegs_lookup();
#endif


    init_zobrist_arr();
    init_TT();


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
        // Find a reasonable first guess no need to filter here
        candidates_arr_t candidates = init_candidates_array();
        first_guess                 = solver(&candidates);
        free(candidates.arr);
    }
    // Together with the search_first_guess.fish it fills the array above
#ifdef SEARCH_FIRST_GUESS
    printf("[%d][%d] = %d, \n", ALLOWED_DIGITS, CODE_LEN, first_guess);
    return 0;
#endif


    int total = 0;

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
                if (pegs.correct == CODE_LEN) {
                    break;
                }

                uint64_t   hash         = filter_candidates(&candidates, pegs, guess);
                tt_entry_t curr_ttentry = TT[hash % TT_TOTAL_ENTRIES];


                // we just shouldnt continue this we know that downstream each decision is the same...
                // but how do we recover the score...
                if (curr_ttentry.hash == hash) {
                    guess = curr_ttentry.code;
                } else {
                    guess                       = solver(&candidates);
                    tt_entry_t new_entry        = { .hash = hash, .code = guess };
                    TT[hash % TT_TOTAL_ENTRIES] = new_entry;
                }
            }
            total += tries;
        }
    }


    float avg_try = (m_float_t) total / (m_float_t) starting_len;

    printf("AVG TRY: %f\n", avg_try);
    return 0;
}

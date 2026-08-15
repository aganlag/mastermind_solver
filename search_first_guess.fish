#!/usr/bin/fish

rm cached_first_guesses.txt
touch cached_first_guesses.txt


set max_digits 7
set max_code_len 6

for i in (seq 0 19)
    for j in (seq 0 19)
        if test $i -gt 0; and test $i -le $max_digits; and test $j -gt 0; and test $j -le $max_code_len
            make -s -B ADDITIONAL_CFLAGS="-DSEARCH_FIRST_GUESS -DALLOWED_DIGITS=$i -DCODE_LEN=$j"
            ./a.out >> cached_first_guesses.txt
        else
            echo "[$i][$j] = -1," >> cached_first_guesses.txt
        end
    end
end


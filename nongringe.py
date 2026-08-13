
from math import log2
import random
import numpy as np
from collections import defaultdict
import copy
import matplotlib.pyplot as plt

class game_state:
    def __init__(self, max_code_len = 4, allowed_digits = 6, code = "RANDOM"):

        self.max_code_len         = max_code_len
        self.allowed_digits       = allowed_digits
        self.running              = True
        self.game_record          = []
        
        self.set_code(code)

    def is_guess_allowed(self, code):

        if not isinstance(code, str):
            return False

        if len(str(code)) != self.max_code_len:
            return False

        for digit in str(code):
            if int(digit) > self.allowed_digits:
                return False

        return True


    def set_code(self, code):

        if self.is_guess_allowed(code):
            self.code = code
        else:
            self.code = "".join([str(random.randint(0, self.allowed_digits - 1)) for _ in range(self.max_code_len)])       


    def make_guess(self, guessed_code):
        
        if not self.is_guess_allowed(guessed_code):
            return False

        self.game_record.append((guessed_code, set_pins(guessed_code, self.code, self.allowed_digits)))

        if guessed_code == self.code:
            self.running = False

        return True


def set_pins(guess, code, allowed_digits):

    assert(len(code) == len(guess))

    white_pins = sum([1 if a == b else 0 for a, b in zip(guess, code)])
    black_pins = sum([min(guess.count(str(a)), code.count(str(a))) for a in range(allowed_digits)]) - white_pins
    return (white_pins, max(0, black_pins) )





ALLOWED_DIGITS = 6
CODE_LENGTH    = 4

def gen_initial_candidates():
    candidates = []
    for a in range(ALLOWED_DIGITS ** CODE_LENGTH):
        c1 = np.base_repr(a, base=ALLOWED_DIGITS).zfill(CODE_LENGTH)
        candidates.append(c1)
    return candidates


SAMPLES = 200


all_candidates = gen_initial_candidates()

def solver(candidates, game_record = []):
    
    new_candidates = []
     
    best_score = float("inf")
    best_move  = 0

    if game_record:
        for c1 in candidates:
            if set_pins(game_record[-1][0], c1, ALLOWED_DIGITS) != game_record[-1][1]:
                continue
            else:
                new_candidates.append(c1)
    else:
        new_candidates = copy.copy(candidates)

    sample = random.sample(
    all_candidates,
    SAMPLES )
    

    for c1 in new_candidates:

        buckets = defaultdict(float)#numbers of buckets are always (allowed_digits + 1)^2/2 - 1
        

        for c2 in new_candidates:
            buckets[set_pins(c1, c2, ALLOWED_DIGITS)] += 1.0

        curr_score = sum([log2(buckets[b]) * (buckets[b] / len(new_candidates)) for b in buckets])
        

        if curr_score < best_score:
            best_score = curr_score
            best_move  = c1 
        #scores[c1] = sum([buckets[b] * (buckets[b]  / len(candidates)) for b in buckets])
        #scores[c1] = buckets[max(buckets, key = lambda x : buckets[x])]
        
    return new_candidates, best_move
    

tot_avg =0

secrets = [a for a in range(ALLOWED_DIGITS ** CODE_LENGTH)]
random.shuffle(secrets)


for x in range(20):
    candidates                = gen_initial_candidates()
    starting_candidates, first_code = solver(candidates)
    avg = 0
    buckets = [0 for x in range (10)]
    for a, s in enumerate(secrets):

        c1 = np.base_repr(a, base=ALLOWED_DIGITS).zfill(CODE_LENGTH)

        game = game_state(allowed_digits= ALLOWED_DIGITS, max_code_len= CODE_LENGTH, code=c1)
        game.make_guess(first_code)

        candidates = copy.copy(starting_candidates)

        while game.running:
            candidates, code = solver(candidates, game.game_record)
            game.make_guess(code)
        buckets[len(game.game_record)] += 1

        avg += len(game.game_record)
        print(f"{a}/{ALLOWED_DIGITS ** CODE_LENGTH} tries: {len(game.game_record)} running avg:{avg / (a + 1) : .2f}")
    if len(game.game_record) == 0:
        print("DIOCANE")
    print(avg / (ALLOWED_DIGITS ** CODE_LENGTH))
    print(buckets)
    tot_avg += avg / (ALLOWED_DIGITS ** CODE_LENGTH)
print(tot_avg/20, "AAA")
#plt.bar(range(len(buckets)), buckets, width=0.9, edgecolor="black")
#print(buckets)
#plt.show()
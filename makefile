
ADDITIONAL_CFLAGS := -DTT_TOTAL_ENTRIES=1000000 -DALLOWED_DIGITS=6 -DCODE_LEN=5 -DDIAGNOSTICS -fopenmp
CC := gcc
CFLAGS := -g -O3 -march=native -Wextra -Wall -Wundef -flto -funroll-loops -ftree-vectorize -ffast-math -mcx16 -lm -lmvec -latomic 
EXECUTABLE := a.out

TEST_EXECUTABLE := test.out


SRCDIR := ./src/
BUILDDIR := ./build/

SRC := $(shell find $(SRCDIR) -name "*.c" | grep -v test.c)
OBJ := $(patsubst $(SRCDIR)%.c,%.o,$(SRC))
DEPS := $(patsubst %.o,%.d,$(OBJ))

TEST_SRC := $(shell find $(SRCDIR) -name "*.c" | grep -v main.c)
TEST_OBJ := $(patsubst $(SRCDIR)%.c,%.o,$(TEST_SRC))
TEST_DEPS := $(patsubst %.o,%.d,$(TEST_OBJ))

.PHONY: test-build
.PHONY: clean


$(EXECUTABLE): $(addprefix $(BUILDDIR), $(OBJ)) 
	$(CC) $(CFLAGS) $(ADDITIONAL_CFLAGS) -o $(EXECUTABLE) $^

$(TEST_EXECUTABLE): $(addprefix $(BUILDDIR), $(TEST_OBJ)) 
	$(CC) $(CFLAGS) $(ADDITIONAL_CFLAGS) -o $(TEST_EXECUTABLE) $^


-include $(addprefix $(BUILDDIR), $(DEPS))
-include $(addprefix $(BUILDDIR), $(TEST_DEPS))

$(BUILDDIR)%.o : $(SRCDIR)%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(ADDITIONAL_CFLAGS) -MMD -c $< -o $@ 

$(BUILDDIR):
	$(shell mkdir -p build)

clean:
	rm -r -f build

test-build: $(TEST_EXECUTABLE)


	

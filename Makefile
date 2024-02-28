CC=gcc
CFLAGS=-Wall -Iinclude
LIBS=

RM=/bin/rm
RMFLAGS=-rf

SRC=src
OBJ=obj
TEST=test

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
TEST_BIN=$(TEST)/main
TEST_SRC=$(TEST)/main.c
SAMPLE_FILE=$(TEST)/samples/testlex.c

all: $(OBJS) $(TEST_BIN)

test: $(TEST_BIN)

objs: $(OBJS)

run: all
	./$(TEST_BIN) $(SAMPLE_FILE)

clean:
	find . -type f | xargs touch
	$(RM) $(RMFLAGS) $(OBJ) $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) $(OBJS) -o $@ $(LIBS)

$(OBJ)/%.o: $(SRC)/%.c $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@ 

$(OBJ):
	mkdir -p $@

.PHONY: all test objs run clean
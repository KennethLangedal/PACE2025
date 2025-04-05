SHELL = /bin/bash

CC = gcc
CFLAGS = -g -std=gnu17 -O3 -march=native -I include -DNDEBUG

OBJ_EXACT = main.o hypergraph.o reductions.o
OBJ_EXACT := $(addprefix bin/, $(OBJ_EXACT))

DEP = $(OBJ_EXACT)
DEP := $(sort $(DEP))

vpath %.c src
vpath %.h include

all : EXACT

-include $(DEP:.o=.d)

EXACT : $(OBJ_EXACT)
	$(CC) $(CFLAGS) -o $@ $^ -lm

bin/%.o : %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

.PHONY : clean
clean :
	rm -f EXACT $(DEP) $(DEP:.o=.d)
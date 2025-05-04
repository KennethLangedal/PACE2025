SHELL = /bin/bash

CC = gcc
CFLAGS = -g -std=gnu17 -O3 -march=native -I include # -DNDEBUG

OBJ_EXACT = main_exact.o hypergraph.o graph.o reductions.o
OBJ_EXACT := $(addprefix bin/, $(OBJ_EXACT))

OBJ_HEURISTIC = main_heuristic.o hypergraph.o graph.o graph_csr.o reductions.o local_search.o chils.o
OBJ_HEURISTIC := $(addprefix bin/, $(OBJ_HEURISTIC))

DEP = $(OBJ_EXACT) $(OBJ_HEURISTIC)
DEP := $(sort $(DEP))

vpath %.c src
vpath %.h include

all : EXACT HEURISTIC

-include $(DEP:.o=.d)

EXACT : $(OBJ_EXACT)
	$(CC) $(CFLAGS) -o $@ $^

HEURISTIC : $(OBJ_HEURISTIC)
	$(CC) $(CFLAGS) -o $@ $^

bin/%.o : %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

.PHONY : clean
clean :
	rm -f EXACT $(DEP) $(DEP:.o=.d)
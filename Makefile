SHELL = /bin/bash

CC = gcc
# CFLAGS = -g -std=gnu17 -march=haswell -O3 -I include -DNDEBUG
CFLAGS = -g -std=gnu17 -march=native -O3 -I include -DNDEBUG
LDFLAGS = -L bin/ -lm -lmwis_reductions
UWRMAXSAT_LIBS = -luwrmaxsat -lcadical -lcominisatps -lmaxpre -lz -lgmp -pthread -lstdc++ -lm -lscip -lsoplex

OBJ_EXACT = main_exact.o hypergraph.o maxsat.o connected_components.o hs_reducer.o hs_reductions.o degree_one.o domination.o \
extended_domination.o counting_rule.o hs_reduction_to_mwis.o
OBJ_EXACT := $(addprefix bin/, $(OBJ_EXACT))

OBJ_HEURISTIC = main_heuristic.o hypergraph.o graph_csr.o local_search.o local_search_hs.o chils.o  connected_components.o \
hs_reducer.o hs_reductions.o degree_one.o domination.o  extended_domination.o counting_rule.o simulated_annealing.o
OBJ_HEURISTIC := $(addprefix bin/, $(OBJ_HEURISTIC))

DEP = $(OBJ_EXACT) $(OBJ_HEURISTIC)
DEP := $(sort $(DEP))

vpath %.c src src/hs_reductions
vpath %.h include include/hs_reductions

all : HEURISTIC EXACT

-include $(DEP:.o=.d)

EXACT : $(OBJ_EXACT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(UWRMAXSAT_LIBS)

HEURISTIC : $(OBJ_HEURISTIC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bin/%.o : %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

.PHONY : clean
clean :
	rm -f EXACT HEURISTIC $(DEP) $(DEP:.o=.d)

run-exact:
	UWRFLAGS="-v0 -no-bin -no-sat -no-par -maxpre-time=60 -scip-cpu=800 -scip-delay=400 -m -bm" ./EXACT < $(GRAPH)
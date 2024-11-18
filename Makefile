SHELL = /bin/bash

CC = gcc
CFLAGS = -std=gnu17 -O3 -march=native -I include -fopenmp -DNDEBUG

OBJ_TEST = main.o graph.o dom_lb.o
OBJ_TEST := $(addprefix bin/, $(OBJ_TEST))

DEP = $(OBJ_TEST)
DEP := $(sort $(DEP))

vpath %.c src
vpath %.h include

all : TEST

-include $(DEP:.o=.d)

TEST : $(OBJ_TEST)
	$(CC) $(CFLAGS) -o $@ $^ -lm

bin/%.o : %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

.PHONY : clean
clean :
	rm -f TEST $(DEP) $(DEP:.o=.d)
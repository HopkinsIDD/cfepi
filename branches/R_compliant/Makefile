# Makefile for building the C language shared library for the CalcMatrixAvg demonstration package.
CC = gcc
RC = R CMD SHLIB
SRC=src
LOPTS= -c -fPIC
# POPTS=-g -O2
ROPTS=$(shell R CMD config --cppflags)
LINKS= -lm
RLINKS= -L/usr/local/lib/R/lib -lR
LOADER = gcc

OBJECTS = {multiple_trials.c,new.counterfactual.c}

.Phony=all
all: library executables
.Phony=library
library: counterfactual.so multiple_trials.so
.Phony=executables
executables: multipleTrials
run: executables
	./multipleTrials
counterfactual.so:
	$(CC) $(POPTS) $(LOPTS) $(ROPTS) $(SRC)/new.counterfactual.c -o counterfactual.so
	$(RC) $(SRC)/new.counterfactual.c
multiple_trials.so:
	$(CC) $(POPTS) $(LOPTS) $(ROPTS) $(SRC)/multiple_trials.c -o multiple_trials.so
	$(RC) $(SRC)/multiple_trials.c

multipleTrials:
	$(CC) $(POPTS) $(ROPTS) $(SRC)/multiple_trials.c -o multipleTrials $(LINKS) $(RLINKS)
	
.Phony=clean
clean:
	-rm *.so src/*.so src/*.o multipleTrials output/*

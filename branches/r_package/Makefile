# Makefile for building the C language shared library for the CalcMatrixAvg demonstration package.
CC = gcc -g3
RC = R CMD SHLIB
SRC=src
LOPTS= -c -fPIC
VOPTS= --track-origins=yes --leak-check=full --show-leak-kinds=all --leak-resolution=low
VLOG=--log-file
# POPTS=-g -O2
ROPTS=$(shell R CMD config --cppflags)
LINKS= -lm
RLINKS= -L/usr/local/lib/R/lib -lR
# RLINKS= -L"C:\Program Files\R\R-3.4.2\lib" -lR
LOADER = gcc
RDBG = MAKEFLAGS="CFLAGS=-g3"# -pg"
## Template for finding files
INTERVENTIONS := $(shell find $(SRC) -name '*_intervention.c')


.Phony=all
all: library

.Phony=library
library: counterfactual.so multiple_trials.so rinterface.so twofunctions.so

counterfactual.so:
	@echo "counterfactual.so"
	$(RDBG) $(RC) $(SRC)/counterfactual.c $(SRC)/twofunctions.c

multiple_trials.so:
	@echo "multiple_trials.so"
	$(RDBG) $(RC) $(SRC)/multiple_trials.c $(SRC)/counterfactual.c $(SRC)/twofunctions.c $(INTERVENTIONS)

twofunctions.so:
	@echo "twofunctions.so"
	$(RDBG) $(RC) $(SRC)/twofunctions.c

rinterface.so:
	@echo "rinterface.so"
	$(RDBG) $(RC) $(SRC)/rinterface.c $(SRC)/counterfactual.c $(SRC)/twofunctions.c $(INTERVENTIONS)

.Phony=clean
clean:
	$(RM) *.so src/*.so src/*.o *.o multipleTrials output/* src/*.dll src/*.rds vgcore*. *.val *.val

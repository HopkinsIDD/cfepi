# Makefile for building the C language shared library for the CalcMatrixAvg demonstration package.
CC = gcc -g3
RC = R CMD SHLIB
SRC=src
LOPTS= -c -fPIC
VOPTS= --track-origins=yes --leak-check=full
# POPTS=-g -O2
ROPTS=$(shell R CMD config --cppflags)
LINKS= -lm
RLINKS= -L/usr/local/lib/R/lib -lR
# RLINKS= -L"C:\Program Files\R\R-3.4.2\lib" -lR
LOADER = gcc

OBJECTS = {multiple_trials.c,counterfactual.c}

.Phony=all
all: library executables
.Phony=library
library: counterfactual.so multiple_trials.so
.Phony=executables
executables: multipleTrials
.Phony=run
run: executables
	./multipleTrials
.Phony=memcheck
memcheck: executables library
	@echo "memcheck:"
	valgrind $(VOPTS) ./multipleTrials
	valgrind $(VOPTS) Rscript tmp.R
counterfactual.so:
	@echo "counterfactual.so"
	# $(CC) $(POPTS) $(LOPTS) $(ROPTS) $(SRC)/counterfactual.c -o counterfactual.so
	$(RC) $(SRC)/counterfactual.c
multiple_trials.so:
	@echo "multiple_trials.so"
	# $(CC) $(POPTS) $(LOPTS) $(ROPTS) $(SRC)/multiple_trials.c -o multiple_trials.so
	# $(RC) $(SRC)/multiple_trials.c $(SRC)/counterfactual.c -o multiple_trials.so
	$(RC) $(SRC)/multiple_trials.c $(SRC)/counterfactual.c

multipleTrials:
	@echo "multipleTrials:"
	$(CC) $(POPTS) $(ROPTS) multiple_trials.c $(SRC)/counterfactual.c -o multipleTrials $(LINKS) $(RLINKS)
.Phony=simultaneous
simultaneous:
	@echo "simultaneous:"
	$(RC) $(SRC)/multiple_trials.c $(SRC)/counterfactual.c
	
.Phony=clean
clean:
	$(RM) *.so src/*.so src/*.o *.o multipleTrials output/* src/*.dll

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
RDBG = MAKEFLAGS="CFLAGS=-g3 -pg"

.Phony=all
all: library executables
.Phony=library
library: counterfactual.so multiple_trials.so rinterface.so twofunctions.so
.Phony=executables
executables: multipleTrials
.Phony=run
run: executables
	./multipleTrials
.Phony=memcheck
memcheck: clean executables library
	@echo "memcheck:"
	valgrind $(VOPTS) $(VLOG)=run.val ./multipleTrials
	R --vanilla -d "valgrind $(VOPTS) $(VLOG)=tmp.val" < tmp.R 
	R --vanilla -d "valgrind $(VOPTS) $(VLOG)=test.val" < test.R
	R --vanilla -d "valgrind $(VOPTS) $(VLOG)=control.val" < test_control.R
.Phony=test2
test2: test2.so
	Rscript tmp3.R
.Phony=callcheck
callcheck: test.so
	Rscript tmp2.R
.Phony=packagetest
packagetest: rinterface.so counterfactual.so twofunctions.so
	Rscript test.R
counterfactual.so:
	@echo "counterfactual.so"
	$(RDBG) $(RC) $(SRC)/counterfactual.c $(SRC)/twofunctions.c
multiple_trials.so:
	@echo "multiple_trials.so"
	$(RDBG) $(RC) $(SRC)/multiple_trials.c $(SRC)/counterfactual.c $(SRC)/twofunctions.c
twofunctions.so:
	@echo "twofunctions.so"
	$(RDBG) $(RC) $(SRC)/twofunctions.c
test.so:
	@echo "test.so"
	$(RDBG) $(RC) $(SRC)/test.c
test2.so:
	@echo "test2.so"
	$(RDBG) $(RC) $(SRC)/test2.c $(SRC)/twofunctions.c

rinterface.so:
	@echo "rinterface.so"
	$(RDBG) $(RC) $(SRC)/rinterface.c $(SRC)/counterfactual.c $(SRC)/twofunctions.c

multipleTrials:
	@echo "multipleTrials:"
	$(CC) $(POPTS) $(ROPTS) multiple_trials.c $(SRC)/counterfactual.c $(SRC)/twofunctions.c -o multipleTrials $(LINKS) $(RLINKS)
.Phony=simultaneous
simultaneous:
	@echo "simultaneous:"
	$(RDBG) $(RC) $(SRC)/multiple_trials.c $(SRC)/counterfactual.c $(SRC)/twofunctions.c
	
.Phony=clean
clean:
	$(RM) *.so src/*.so src/*.o *.o multipleTrials output/* src/*.dll vgcore*. *.val *.val

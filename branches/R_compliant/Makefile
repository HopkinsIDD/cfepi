# Makefile for building the C language shared library for the CalcMatrixAvg demonstration package.
CC = gcc
SRC=src
LOPTS= -c -fPIC
POPTS=-g -O2
ROPTS = -fPIC -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORITFY_SOURCE=2 -g -c
LINKS= -lgsl -lgslcblas -lm
LOADER = gcc

OBJECTS = {multiple_trials.c,counterfactual.c}

.Phony=all
all: library executables Rlib
.Phony=library
library: counterfactual.so multiple_trials.so
.Phony=executables
executables: multipleTrials
.Phony=Rlib
Rlib: test.so test2.so
run: executables
	./multipleTrials
counterfactual.so:
	$(CC) $(POPTS) $(LOPTS) $(SRC)/counterfactual.c -o counterfactual.so $(LINKS)
multiple_trials.so:
	$(CC) $(POPTS) $(LOPTS) $(SRC)/multiple_trials.c -o multiple_trials.so $(LINKS)
multipleTrials:
	$(CC) $(POPTS) $(SRC)/multiple_trials.c -o multipleTrials $(LINKS)
test.so:
	$(CC) $(ROPTS) $(SRC)/multiple_trials.c -o test.so $(LINKS)
test2.so:
	$(CC) $(ROPTS) $(SRC)/counterfactual.c -o test2.so $(LINKS)
	
.Phony=clean
clean:
	-rm *.so multipleTrials output/*

#gcc -std=gnu99 -I/usr/share/R/include -DNDEBUG      -fpic  -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -g  -c counterfactual.c -o counterfactual.o
#gcc -std=gnu99 -I/usr/share/R/include -DNDEBUG      -fpic  -g -O2 -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -g  -c multiple_trials.c -o multiple_trials.o
#gcc -std=gnu99 -shared -L/usr/lib/R/lib -Wl,-Bsymbolic-functions -Wl,-z,relro -o counterfactual.so counterfactual.o multiple_trials.o -L/usr/lib/R/lib -lR

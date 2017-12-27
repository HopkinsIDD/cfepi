#ifndef RINTERFACE_H_
#define RINTERFACE_H_

#include <Rinternals.h>

void R2cmat(SEXP, double**, int*, int*);
void R2cvecint(SEXP, int**, int*);
void R2cvecdouble(SEXP, double**, int*);
void R2cdouble(SEXP, double*);
void R2cint(SEXP, int*);
void R2cstring(SEXP, char**);
void c2Rdataframe(double*,int,int,SEXP*);
SEXP setupCounterfactualAnalysis(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);

#endif //RINTERFACE_H_

#ifndef RINTERFACE_H_
#define RINTERFACE_H_

#include <Rinternals.h>

void R2cmat(SEXP, double**, int*, int*);
void R2cvecint(SEXP, int**, int*);
void R2cvecdouble(SEXP, double**, int*);
double R2cdouble(SEXP);
int R2cint(SEXP);
void R2cstring(SEXP, char**);
void c2Rdataframe(double*,int,int,SEXP*);
SEXP setupCounterfactualAnalysis(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
SEXP runIntervention(SEXP,SEXP,SEXP,SEXP,SEXP,SEXP,SEXP,SEXP,SEXP);
SEXP readDirectory(SEXP,SEXP);

#endif //RINTERFACE_H_

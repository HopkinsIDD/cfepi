#ifndef COUNTERFACTUAL_H_
#define COUNTERFACTUAL_H_

#include "types.h"
#include "twofunctions.h"

#define CONSTRUCT_DEBUG 0
#define RUN_DEBUG 0

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))
#define MAX(x,y) (x) > (y) ? (x) : (y);
#define MIN(x,y) (x) < (y) ? (x) : (y);


void runCounterfactualAnalysis(char*,person_t*,var_t,step_t, double*, double*,char*,char*);
void runFastCounterfactualAnalysis(person_t*,var_t,step_t, double*, double*,char*,char*);
void constructTimeSeries(person_t* ,var_t, step_t,saved_beta_t reduceBeta,saved_susceptible_t eliminateSusceptibles,char*,char*,char*);

#endif //COUNTERFACTUAL_H_

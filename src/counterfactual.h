#ifndef COUNTERFACTUAL_H_
#define COUNTERFACTUAL_H_

#include <stdint.h>
#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))
#define MAX(x,y) (x) > (y) ? (x) : (y);
#define MIN(x,y) (x) < (y) ? (x) : (y);

// Custom types for storage efficiency
# define bool_t uint8_t
# define var_t uint8_t
# define person_t uint32_t
# define step_t uint16_t

void runCounterfactualAnalysis(char*,person_t*,var_t,step_t, double*, double*,char*,char*);
void runFastCounterfactualAnalysis(person_t*,var_t,step_t, double*, double*,char*,char*);
void constructTimeSeries(person_t* ,var_t, step_t,bool_t (*reduceBeta)(step_t,person_t,person_t,var_t,var_t),void (*eliminateSusceptible)(var_t**,step_t,step_t,person_t),char*,char*,char*);
//Fake R functions
void sample(person_t**, person_t,person_t);
void swap(person_t*,person_t*);
void no_interventionSusceptible(var_t**,step_t,step_t,person_t);
bool_t no_interventionBeta(step_t,person_t,person_t,var_t,var_t);

#endif //COUNTERFACTUAL_H_

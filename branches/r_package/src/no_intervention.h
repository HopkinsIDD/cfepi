#ifndef NO_INTERVENTION_H_
#define NO_INTERVENTION_H_
#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"
typedef struct {
} data_beta_no_t;

param_beta_t param_no_beta();
void free_param_no_beta(param_beta_t);
typedef struct {
} data_susceptible_no_t;

param_susceptible_t param_no_susceptible();
void free_param_no_susceptible(param_susceptible_t);
bool_t no_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars);
void no_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars);
#endif //NO_INTERVENTION_H_

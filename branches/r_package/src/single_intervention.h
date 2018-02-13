#ifndef SINGLE_INTERVENTION_H_
#define SINGLE_INTERVENTION_H_
#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"
typedef struct {
} data_beta_single_t;

param_beta_t param_single_beta();
void free_param_single_beta(param_beta_t);
typedef struct {
  step_t time;
  float rate;
} data_susceptible_single_t;

param_susceptible_t param_single_susceptible(step_t , float );
void free_param_single_susceptible(param_susceptible_t);
bool_t single_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars);
void single_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars);
#endif //SINGLE_INTERVENTION_H_

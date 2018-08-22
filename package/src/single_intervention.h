#ifndef SINGLE_INTERVENTION_H_
#define SINGLE_INTERVENTION_H_
#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"

typedef struct {
  step_t time;
  float rate;
  var_t move_to;
  var_t nfrom;
  var_t* move_from;
} data_susceptible_single_t;

param_susceptible_t param_single_susceptible(step_t , float ,var_t,var_t*,var_t);
bool_t free_param_single_susceptible(param_susceptible_t);
bool_t single_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars);
void single_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars);
#endif //SINGLE_INTERVENTION_H_

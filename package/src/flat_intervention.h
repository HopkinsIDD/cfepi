#ifndef FLAT_INTERVENTION_H_
#define FLAT_INTERVENTION_H_
#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"
typedef struct {
  step_t start_time;
  float rate;
} data_beta_flat_t;

param_beta_t param_flat_beta(step_t, float);
bool_t free_param_flat_beta(param_beta_t);

bool_t free_param_flat_susceptible(param_susceptible_t);
bool_t flat_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars);
#endif //FLAT_INTERVENTION_H_

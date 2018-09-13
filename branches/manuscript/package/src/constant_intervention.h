#ifndef CONSTANT_INTERVENTION_H_
#define CONSTANT_INTERVENTION_H_
#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"

typedef struct {
  step_t time;
  float rate;
  var_t move_to;
  var_t nfrom;
  var_t move_from;
  person_t* targets;
  person_t naffected;
} data_susceptible_constant_t;

param_susceptible_t param_constant_susceptible(step_t , float, person_t, float ,var_t,var_t,var_t);
bool_t free_param_constant_susceptible(param_susceptible_t);
void constant_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars);
#endif //CONSTANT_INTERVENTION_H_

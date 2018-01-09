#ifndef INTERVENTION_H_
#define INTERVENTION_H_

#include "types.h"

typedef struct{
  int time;
} param_beta_t;

typedef bool_t (*beta_t)(step_t, person_t, person_t, var_t, var_t, param_beta_t);

typedef struct {
  beta_t func;
  param_beta_t pars;
} saved_beta_t;

saved_beta_t partially_evaluate_beta(beta_t, param_beta_t);
bool_t invoke_beta_t(saved_beta_t,step_t,person_t,person_t,var_t,var_t);

bool_t interventionBeta(step_t,person_t,person_t,var_t,var_t,param_beta_t);
bool_t no_interventionBeta(step_t,person_t,person_t,var_t,var_t,param_beta_t);

typedef struct{
  int time;
} param_susceptible_t;

typedef void (*susceptible_t)(var_t**, step_t, step_t, person_t,param_susceptible_t);

typedef struct {
  susceptible_t func;
  param_susceptible_t pars;
} saved_susceptible_t;

saved_susceptible_t partially_evaluate_susceptible(susceptible_t, param_susceptible_t);
void invoke_susceptible_t(saved_susceptible_t,var_t**,step_t,step_t,person_t);

void no_interventionSusceptibles(var_t**,step_t,step_t,person_t,param_susceptible_t);
void interventionSusceptibles(var_t**,step_t,step_t,person_t,param_susceptible_t);
/*
void parametrized_interventionSusceptibles(var_t**,step_t,step_t,person_t, param_beta_t);
bool_t parametrized_intervenionBeta(step_t,person_t,person_t,var_t,var_t, param_beta_t);

void no_interventionSusceptibles(var_t**,step_t,step_t,person_t);
bool_t no_interventionBeta(step_t,person_t,person_t,var_t,var_t);
*/

#endif //INTERVENTION_H_


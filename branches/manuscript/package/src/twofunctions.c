#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"

saved_beta_t partially_evaluate_beta(beta_t func, param_beta_t pars){
  saved_beta_t rc;
  rc.func = func;
  rc.pars = pars;
  return(rc);
}

bool_t invoke_beta_t(saved_beta_t func, step_t time,person_t person1, person_t person2, var_t var1, var_t var2){
  return(func.func(time,person1,person2,var1,var2,func.pars));
}

saved_susceptible_t partially_evaluate_susceptible(susceptible_t func, param_susceptible_t pars){
  saved_susceptible_t rc;
  rc.func = func;
  rc. pars = pars;
  return(rc);
}
void invoke_susceptible_t(saved_susceptible_t func, var_t** state, step_t time, step_t ntime, person_t npop){
  return(func.func(state,time,ntime,npop,func.pars));
}

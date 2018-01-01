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

bool_t no_interventionBeta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars){
  return(1);
}

bool_t interventionBeta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars){
  if(time > pars.time){
    return(0);
  }
  return(1);
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

void no_interventionSusceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  return;
}
void interventionSusceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  if(time > pars.time){
    printf("Weird\n");
  }
}

/*
int main(int argc, char** argv){
  saved_beta_t evaluated_beta;
  param_beta_t test_pars;
  beta_t test_beta;
  
  test_pars.time = 5;
  test_beta = *interventionBeta;
  evaluated_beta = partially_evaluate(test_beta,test_pars);
  printf("%d\n%d\n",invoke_beta_t(evaluated_beta,0,0,0,0,0),invoke_beta_t(evaluated_beta,7,0,0,0,0));
  
}
*/

/*
void no_interventionSusceptible(var_t** states, step_t time, step_t ntime, person_t npop){
}

bool_t no_interventionBeta(step_t itime,person_t iperson1,person_t iperson2,var_t ivar1,var_t ivar2){
  return(1);
}
*/

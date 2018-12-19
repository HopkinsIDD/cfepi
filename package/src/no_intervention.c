#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "no_intervention.h"
#include <R.h>
#include <R_ext/Print.h>
#include <Rmath.h>
param_beta_t param_no_beta(){
  param_beta_t rc;
  strcpy(rc.type , "no");
  rc.data = malloc(1 * sizeof(data_beta_no_t));
  return(rc);
}

bool_t free_param_no_beta(param_beta_t rc){
  if(strcmp(rc.type,"no")!=0){
    Rf_error("Attempting to free a param_beta_t with the wrong destructor\n");
    return(1);
  }
  free(rc.data);
  return(0);
}

// Finish writing these
bool_t no_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars){
  return(1);
}

param_susceptible_t param_no_susceptible(){
  param_susceptible_t rc;
  strcpy(rc.type,"no");
  rc.data = malloc(1 * sizeof(data_susceptible_no_t));
  return(rc);
}

bool_t free_param_no_susceptible(param_susceptible_t rc){
  if(strcmp(rc.type,"no")!=0){
    Rf_error("Attempting to free a param_susceptible_t with the wrong destructor\n");
    return(1);
  }
  free(rc.data);
  return(0);
}

// Finish writing these
void no_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  return;
}


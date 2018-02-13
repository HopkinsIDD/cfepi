#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flat_intervention.h"

#include <R.h>
#include <Rmath.h>
param_beta_t param_flat_beta(step_t start_time, float rate){
  param_beta_t rc;
  strcpy(rc.type , "flat");
  rc.data = malloc(1 * sizeof(data_beta_flat_t));
  (* ( (data_beta_flat_t*) rc.data) ).start_time = start_time;
  (* ( (data_beta_flat_t*) rc.data) ).rate = rate;
  return(rc);
};

void free_param_flat_beta(param_beta_t rc){
  if(strcmp(rc.type,"flat")!=0){
    fprintf(stderr,"Attempting to free a param_beta_t with the wrong destructor\n");
    exit(1);
  }
  free(rc.data);
};

// Finish writing these
bool_t flat_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars){
  if(
    ( time >= (*( (data_beta_flat_t*) pars.data)).start_time ) && 
    (runif(0.,1.) < (*( (data_beta_flat_t*) pars.data)).rate)
  ){
    return(0);
  }
  return(1);
}

param_susceptible_t param_flat_susceptible(){
  param_susceptible_t rc;
  strcpy(rc.type,"flat");
  rc.data = malloc(1 * sizeof(data_susceptible_flat_t));
  return(rc);
};

void free_param_flat_susceptible(param_susceptible_t rc){
  if(strcmp(rc.type,"flat")!=0){
    fprintf(stderr,"Attempting to free a param_susceptible_t with the wrong destructor\n");
    exit(1);
  }
  free(rc.data);
};

// Finish writing these
void flat_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  return;
}


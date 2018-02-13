#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "single_intervention.h"
param_beta_t param_single_beta(){
  param_beta_t rc;
  strcpy(rc.type , "single");
  rc.data = malloc(1 * sizeof(data_beta_single_t));
  return(rc);
};

void free_param_single_beta(param_beta_t rc){
  if(strcmp(rc.type,"single")!=0){
    fprintf(stderr,"Attempting to free a param_beta_t with the wrong destructor\n");
    exit(1);
  }
  free(rc.data);
};

// Finish writing these
bool_t single_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars){
  return(1);
}

param_susceptible_t param_single_susceptible(step_t time, float rate){
  param_susceptible_t rc;
  strcpy(rc.type,"single");
  rc.data = malloc(1 * sizeof(data_susceptible_single_t));
  (* ( (data_susceptible_single_t *) rc.data) ).time = time;
  (* ( (data_susceptible_single_t *) rc.data) ).rate = rate;
  return(rc);
};

void free_param_single_susceptible(param_susceptible_t rc){
  if(strcmp(rc.type,"single")!=0){
    fprintf(stderr,"Attempting to free a param_susceptible_t with the wrong destructor\n");
    exit(1);
  }
  free(rc.data);
};

// Finish writing these
void single_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  return;
}


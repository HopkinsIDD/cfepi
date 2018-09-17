#ifndef INTERVENTION_H_
#define INTERVENTION_H_

#include "types.h"

//Saved parameters for the two main functions
typedef struct{
  char type[256];     //Which type of beta_t this is for.  This should be a string of the prefix of the beta_t.  The beta_t function will check this and if it doesn't 
  void* data;
} param_beta_t;

typedef struct{
  char type[256];     //Which type of beta_t this is for.  This should be a string of the prefix of the beta_t.  The beta_t function will check this and if it doesn't 
  void* data;
} param_susceptible_t;

//typedef the two functions for eas of use
typedef bool_t (*beta_t)(step_t, person_t, person_t, var_t, var_t, param_beta_t);
typedef void (*susceptible_t)(var_t**, step_t, step_t, person_t,param_susceptible_t);

//Make saved versions so we can set the parameters ahead of time
typedef struct {
  beta_t func;
  param_beta_t pars;
} saved_beta_t;

typedef struct {
  susceptible_t func;
  param_susceptible_t pars;
} saved_susceptible_t;

//make a function to partially evaluate
saved_beta_t partially_evaluate_beta(beta_t, param_beta_t);
saved_susceptible_t partially_evaluate_susceptible(susceptible_t, param_susceptible_t);

//make a function to finish evaluation
bool_t invoke_beta_t(saved_beta_t,step_t,person_t,person_t,var_t,var_t);
void invoke_susceptible_t(saved_susceptible_t,var_t**,step_t,step_t,person_t);

//Declare actual functions and corresponding param constructors and destructors for beta_t and susceptible_t functions each in their own separate file.
//TODO Write a perl script (or macro) to make this easier 

//Fake R functions
void sample(person_t**, person_t,person_t);
void swap(person_t*,person_t*);

#endif //INTERVENTION_H_

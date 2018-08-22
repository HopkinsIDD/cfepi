#include <stdlib.h>
#include <stdio.h>
#include "twofunctions.h"

// R specific headers
#include <R.h>
//#include <R_ext/Print.h>
//#include <Rinternals.h>
#include <Rmath.h>
//#include <R_ext/Rdynload.h>
//#include <Rdefines.h>

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

// see header file for type definitions.  All custom types are integers of a particular size.
// A utility function to swap to people
void swap (person_t *a, person_t *b){
  person_t temp = *a;
  *a = *b;
  *b = temp;
}
/*
 * person_t* (*output) an integer vector to return with the sampes in it
 * person_t          n The original number to sample from
 * person_t          k The number of samples to draw
*/
void sample(person_t* *output, person_t n, person_t k){
  person_t i,j;
  for (i = 0; i < k; i++){
    // Pick a random index from 0 to i
    j = runif(i,n);
    if(j >= n){j = i;}
    // Swap arr[i] with the element at random index
    swap(&(*output)[i], &(*output)[j]);
  }
}

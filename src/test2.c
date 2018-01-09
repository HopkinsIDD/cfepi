#include <R.h>
#include <Rmath.h>
#include <Rinternals.h>
#include "twofunctions.h"

SEXP test(){
  param_beta_t param_beta;
  param_susceptible_t param_susceptible;
  saved_beta_t no_intervention_reduceBeta;
  saved_susceptible_t no_intervention_eliminateSusceptibles;

  param_beta.time = 5;
  param_susceptible.time = 5;
  no_intervention_reduceBeta = partially_evaluate_beta(
    &no_interventionBeta,
    param_beta
  );
  /*no_intervention_eliminateSusceptibles = partially_evaluate_susceptible(
    no_intervention_unparametrized_eliminateSusceptibles,
    param_susceptible
  );*/
  bool_t test = invoke_beta_t(
    no_intervention_reduceBeta,
    0,
    0,
    1,
    0,
    1);
  printf("test is %d\n");
  return(R_NilValue);
}

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "twofunctions.h"
#include "counterfactual.h"

// R specific headers
#include <R.h>
//#include <Rinternals.h>
#include <Rmath.h>
//#include <R_ext/Rdynload.h>
//#include <Rdefines.h>

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))

void testfun();

int vaccination_occurred;
int vaccination_time;
double vaccination_percent;

double distancing_percent;
int distancing_time;

int main(){
  int trial,ntrial;
  var_t nvar,var,var2;
  step_t ntime;
  person_t npop;
  double beta,gamma;
  person_t* init;
  double* transitions;
  double* interactions;
  char fn[1000];
  char tfn[1000];
  char ifn[1000];
  beta_t no_intervention_unparametrizedBeta, intervention_unparametrizedBeta;
  susceptible_t no_intervention_unparametrizedSusceptibles, intervention_unparametrizedSusceptibles;
  param_beta_t beta_pars;
  param_susceptible_t susceptible_pars;
  saved_beta_t no_intervention_reduceBeta, intervention_reduceBeta;
  saved_susceptible_t no_intervention_eliminateSusceptibles, intervention_eliminateSusceptibles;

  //get pointers to the original functions
  no_intervention_unparametrizedBeta = *no_interventionBeta;
  intervention_unparametrizedBeta = *interventionBeta;
  no_intervention_unparametrizedSusceptibles = *no_interventionSusceptibles;
  intervention_unparametrizedSusceptibles = *interventionSusceptibles;
  
  //Set the parameters, this will normally be done from within R.
  beta_pars.time = 5;
  susceptible_pars.time = 5;
  
  no_intervention_reduceBeta = partially_evaluate_beta(no_intervention_unparametrizedBeta,beta_pars);
  intervention_reduceBeta = partially_evaluate_beta(intervention_unparametrizedBeta,beta_pars);
  no_intervention_eliminateSusceptibles = partially_evaluate_susceptible(no_intervention_unparametrizedSusceptibles,susceptible_pars);
  intervention_eliminateSusceptibles = partially_evaluate_susceptible(intervention_unparametrizedSusceptibles,susceptible_pars);
  
  GetRNGstate();
  nvar = 3;
  init = calloc(nvar,sizeof(person_t));
  // Real Values
  /*
  ntime = 365;
  init[0] = 399990;
  // init[0] = 190;
  init[1] = 10;
  init[2] = 0;
  ntrial = 200;
  */
  //Testing Values
  ntime = 100;
  init[0] = 3990;
  init[1] = 10;
  init[2] = 0;
  ntrial = 1;

  npop = 0;
  for(var = 0; var < nvar; ++var){
    npop += init[var];
    printf("Category %d: %d\n",var,init[var]);
  }
  printf("Total Population: %d\n",npop);

  vaccination_time = 30;
  vaccination_percent = .01;

  gamma = .2;
  beta = .4;

  distancing_percent = .01;
  distancing_time = 30;
  printf("Expected R0 is %f\n",beta/gamma);

  transitions = calloc(nvar*nvar,sizeof(double));
  interactions = calloc(nvar*nvar,sizeof(double));
  //Only store the positive elements of the transition matrix
  transitions[IND(1,2,nvar)] = gamma;
  //Only store the positive elements of the interaction matrix
  interactions[IND(0,1,nvar)] = beta/npop;

  for(trial = 0; trial < ntrial; ++trial){
    printf("Running Trial %d\n",trial);
    printf("init:");
    for(var = 0; var < nvar; ++var){
      printf(" %d",init[var]);
    }
    printf("\nnvar: %d\nntime %d\ntransitions:\n",nvar,ntime);
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        printf(" %f",transitions[IND(var,var2,nvar)]);
      }
      printf("\n");
    }
    printf("\ninteractions:\n");
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        printf(" %f",interactions[IND(var,var2,nvar)]);
      }
      printf("\n");
    }
    printf("\ntfn: %s\nifn: %s\n",tfn,ifn);
    vaccination_occurred = 0;
    sprintf(ifn,"output/interaction.0.%d.csv",trial);
    sprintf(tfn,"output/transition.0.%d.csv",trial);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
    sprintf(ifn,"output/interaction.1.%d.csv",trial);
    sprintf(tfn,"output/transition.1.%d.csv",trial);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
    // return(0);
  }

  for(trial = 0; trial < ntrial; ++trial){
    printf("Evaluating Trial %d\n",trial);
    sprintf(ifn,"output/interaction.0.%d.csv",trial);
    sprintf(tfn,"output/transition.0.%d.csv",trial);
    sprintf(fn,"output/%s.%d.%d.csv","no_intervention",0,trial);
    fflush(stdout);
    constructTimeSeries(
      init,
      nvar,
      ntime,
      no_intervention_reduceBeta,
      no_intervention_eliminateSusceptibles,
      tfn,
      ifn,
      fn
    );
    sprintf(fn,"output/%s.%d.%d.csv","intervention",0,trial);
    return(0);
    constructTimeSeries(
      init,
      nvar,
      ntime,
      intervention_reduceBeta,
      intervention_eliminateSusceptibles,
      tfn,
      ifn,
      fn
    );
    sprintf(ifn,"output/interaction.1.%d.csv",trial);
    sprintf(tfn,"output/transition.1.%d.csv",trial);
    sprintf(fn,"output/%s.%d.%d.csv","no_intervention",1,trial);
    constructTimeSeries(
      init,
      nvar,
      ntime,
      no_intervention_reduceBeta,
      no_intervention_eliminateSusceptibles,
      tfn,
      ifn,
      fn
    );
  }

  free(transitions);
  free(interactions);
  free(init);
  PutRNGstate();
}

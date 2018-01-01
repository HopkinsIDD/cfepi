#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

void interventionSusceptible(var_t**,step_t,step_t,person_t);
bool_t interventionBeta(step_t,person_t,person_t,var_t,var_t);
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
      &no_interventionBeta,
      &no_interventionSusceptible,
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
      &interventionBeta,
      &interventionSusceptible,
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
      &no_interventionBeta,
      &no_interventionSusceptible,
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

void interventionSusceptible(var_t** states, step_t time, step_t ntime, person_t npop){
}

bool_t interventionBeta(step_t itime,person_t iperson1,person_t iperson2,var_t ivar1,var_t ivar2){
  return(1);
}

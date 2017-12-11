						\
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcg_variants.h>
#include <tgmath.h>
#include <assert.h>
#include "counterfactual.c"
#include <time.h>

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))

int no_interventionBeta(int,int,int,int,int);
void no_interventionSusceptible(int**,int,int,int);
int interventionBeta(int,int,int,int,int);
void interventionSusceptible(int**,int,int,int);

int vaccination_occurred;
int vaccination_time;
float vaccination_percent;

float distancing_percent;
int distancing_time;

int main(int argc, char** argv){
  if(argc != 0+1){
    fprintf(stderr,"Wrong number of arguments provided.\n");
  }

  int nvar,ntime,npop,person,trial,ntrial,var;
  int nondeterministic_seed = 0;
  float beta,gamma;
  int* init;
  float* transitions;
  float* interactions;
  char fn[1000];
  char tfn[1000];
  char ifn[1000];
  const gsl_rng_type * T;

  gsl_rng * rng;
  gsl_rng_env_setup();
  T = gsl_rng_default;
  rng = gsl_rng_alloc (T);


  //Deal with seeding random number generator:
  if (nondeterministic_seed) {
    // Seed with external entropy

    uint64_t seeds[2];
    // This line was originally uncommented...
    // entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg32_srandom(seeds[0], seeds[1]);
  } else {
    // Seed with a fixed constant

    pcg32_srandom(time(NULL), 54u);
  }
  
  
  nvar = 3;
  init = calloc(nvar,sizeof(int));
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
  ntrial = 100;

  npop = 0;
  for(var = 0; var < nvar; ++var){
    npop += init[var];
    printf("Category %d: %d\n",var,init[var]);
  }
  printf("Total Population: %d\n",npop);

  vaccination_time = 30;
  vaccination_percent = .01;

  gamma = .1;
  beta = .2;

  distancing_percent = .01;
  distancing_time = 30;
  printf("Expected R0 is %f\n",beta/gamma);

  transitions = calloc(nvar*nvar,sizeof(float));
  interactions = calloc(nvar*nvar,sizeof(float));
  //Only store the positive elements of the transition matrix
  transitions[IND(1,2,nvar)] = gamma;
  //Only store the positive elements of the interaction matrix
  interactions[IND(0,1,nvar)] = beta/npop;


  for(trial = 0; trial < ntrial; ++trial){
    printf("Running Trial %d\n",trial);
    vaccination_occurred = 0;
    sprintf(ifn,"output/interaction.0.%d.csv",trial);
    sprintf(tfn,"output/transition.0.%d.csv",trial);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn,rng);
    sprintf(ifn,"output/interaction.1.%d.csv",trial);
    sprintf(tfn,"output/transition.1.%d.csv",trial);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn,rng);
  }

  for(trial = 0; trial < ntrial; ++trial){
    printf("Evaluating Trial %d\n",trial);
    sprintf(ifn,"output/interaction.0.%d.csv",trial);
    sprintf(tfn,"output/transition.0.%d.csv",trial);
    sprintf(fn,"output/%s.%d.%d.csv","no_intervention",0,trial);
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
  gsl_rng_free(rng);
}

void no_interventionSusceptible(int** states,int time,int ntime,int npop){
}

int no_interventionBeta(int itime,int iperson1,int iperson2,int ivar1,int ivar2){
  return(1);
}

void interventionSusceptible(int** states,int time,int ntime,int npop){
}

int interventionBeta(int itime,int iperson1,int iperson2,int ivar1,int ivar2){
  if(itime <= distancing_time){
    return(1);
  }
  if(ldexp(pcg32_random(), -32) < distancing_percent){
    return(0);
  }
  return(1);
}

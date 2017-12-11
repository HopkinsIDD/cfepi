						\
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pcg_variants.h>
#include <tgmath.h>
#include <assert.h>
#include "counterfactual.c"

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))

void social_distancingSusceptible(int**, int,int,int);
int social_distancingBeta(int,int,int,int,int);

float distancing_percent;
int distancing_time;

int main(int argc, char** argv){
  if(argc != 0+1){
    fprintf(stderr,"Wrong number of arguments provided.\n");
  }

  int nvar,ntime,npop;
  int nondeterministic_seed = 0;
  float beta,gamma;
  int* init;
  float* transitions;
  float* interactions;


  //Deal with seeding random number generator:
  if (nondeterministic_seed) {
    // Seed with external entropy

    uint64_t seeds[2];
    // This line was originally uncommented...
    // entropy_getbytes((void*)seeds, sizeof(seeds));
    pcg32_srandom(seeds[0], seeds[1]);
  } else {
    // Seed with a fixed constant

    pcg32_srandom(42u, 54u);
  }
  
  
  nvar = 3;
  ntime = 200;
  init = calloc(nvar,sizeof(int));
  init[0] = 3990;
  // init[0] = 190;
  init[1] = 10;
  init[2] = 0;
  npop = 4000;

  gamma = .1;
  beta = .2;

	distancing_percent = .2;
	distancing_time = 30;
  printf("Expected R0 is %f\n",beta/gamma);

  transitions = calloc(nvar*nvar,sizeof(float));
  interactions = calloc(nvar*nvar,sizeof(float));
  //Only store the positive elements of the transition matrix
  transitions[IND(1,2,nvar)] = gamma;
  //Only store the positive elements of the interaction matrix
  interactions[IND(0,1,nvar)] = beta/npop;

  // runCounterfactualAnalysis("Full",init,nvar,ntime,transitions,interactions);
  constructTimeSeries(init,nvar,ntime,&social_distancingBeta,&social_distancingSusceptible,"social_distancing.csv");

  free(transitions);
  free(interactions);
  free(init);
}

void social_distancingSusceptible(int** states,int time,int ntime,int npop){
}
int social_distancingBeta(int itime,int iperson1,int iperson2,int ivar1,int ivar2){
  if(ldexp(pcg32_random(), -32) < distancing_percent){
    if(itime > distancing_time){
      return(0);
    }
  }
  return(1);
}
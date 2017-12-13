#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pcg_variants.h"
#include <tgmath.h>
#include <assert.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

// R specific headers
//#include <R.h>
//#include <Rinternals.h>
//#include <Rmath.h>
//#include <R_ext/Rdynload.h>
//#include <Rdefines.h>

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))

// ( (int) pcg32_boundedrand_r(&rng,INT_MAX-1) + INT_MIN);

void runCounterfactualAnalysis(char*,int*, int, int, float*, float*,char*,char*,gsl_rng*);
void runFullCounterfactualAnalysis(int*, int, int, float*, float*,char*,char*,gsl_rng*);
void runFastCounterfactualAnalysis(int*, int, int, float*, float*,char*,char*,gsl_rng*);
void constructTimeSeries(int* ,int, int,int (*reduceBeta)(int,int,int,int,int),void (*eliminateSusceptible)(int**, int,int,int),char*,char*,char*);

void runCounterfactualAnalysis(char* type, int* init,int nvar, int ntime, float* transitions, float* interactions,char* tfname, char* ifname,gsl_rng *rng){
  if(strcmp(type,"Full")==0){
    runFullCounterfactualAnalysis(init,nvar,ntime,transitions,interactions,tfname,ifname,rng);
    return;
  }
  if(strcmp(type,"Fast")==0){
    runFastCounterfactualAnalysis(init,nvar,ntime,transitions,interactions,tfname,ifname,rng);
    return;
  }
  fprintf(stderr,"type %s is invalid\n",type);
  exit(1);
}

void runFastCounterfactualAnalysis(int* init,int nvar, int ntime, float* transitions, float* interactions,char* tfname,char* ifname,gsl_rng *rng){
  int npop,var1,var2,time,person1,person2,counter,tmp,interaction,ninteraction,index;
  double test;
  FILE *tfp;
  FILE *ifp;
  npop = 0;
  for(var1 = 0; var1< nvar; ++var1){
    npop = npop + init[var1];
  }
  int* actualTransitions;
  int* actualInteractions;
  int** possibleStates;
  int** nextPossibleStates;
  int* targets;
  //Note: To help with memory useage, we may want to modify these to be more efficient
  // actualTransitions = malloc(nvar*nvar*ntime*npop*sizeof(int));
  // actualInteractions = malloc(nvar*nvar*ntime*npop*npop*sizeof(int));
  possibleStates = calloc(nvar,sizeof(int*));
  for(var1 = 0; var1 < nvar; ++ var1){
    possibleStates[var1] = calloc(npop,sizeof(int));
  }
  nextPossibleStates = malloc(nvar*sizeof(int*));
  for(var1 = 0; var1 < nvar; ++ var1){
    nextPossibleStates[var1] = calloc(npop,sizeof(int));
  }
  //Construct initial states:
  counter = 0;
  for(var1 = 0; var1 < nvar; ++ var1){
    for(person1 = counter; person1 < (counter + init[var1]); ++ person1){
      nextPossibleStates[var1][person1] += 1;
    }
    counter = counter + init[var1];
  }
  assert(counter == npop);

  // printf("Looping Variables:\n\tnvar is %d\n\tntime is %d\n\tnpop is %d\n",nvar,ntime,npop);
  tfp = fopen(tfname,"w");
  ifp = fopen(ifname,"w");

  targets = malloc(npop * sizeof(int));
  for(person1 = 0; person1 < npop; ++person1){
    targets[person1] = person1;
  }

  for(time = 0; time < ntime; ++time){
    for(var1 = 0; var1 < nvar; ++var1){
      for(person1=0;person1<npop;++person1){
	possibleStates[var1][person1] = nextPossibleStates[var1][person1];
      }
    }
      
    for(person1=0;person1<npop;++person1){
      for(var1 = 0; var1 <nvar; ++var1){
	if(possibleStates[var1][person1] == 0) continue;
	for(var2 = 0; var2 <nvar; ++ var2){
	  if(transitions[IND(var1,var2,nvar)] > 0){
	    if(( (float) ldexp(pcg32_random(), -32)) < transitions[IND(var1,var2,nvar)]){
	      // fprintf(tfp,"%d,%d,%d,%d,%d\n",var1,var2,time,person1,actualTransitions[IND4(var1,var2,time,person1,nvar,ntime,npop)]);
	      // fprintf(tfp,"%d:%d:%d->%d\n",time,person1,var1,var2);
              // fprintf(tfp,"t:");
	      // printf("%d:%d:%d->%d\n",time,person1,var1,var2);
	      fwrite(&time,sizeof(time),1,tfp);
	      fwrite(&person1,sizeof(person1),1,tfp);
	      fwrite(&var1,sizeof(var1),1,tfp);
	      fwrite(&var2,sizeof(var2),1,tfp);
	      nextPossibleStates[var2][person1] += 1;
	    }
          }
	  if(interactions[IND(var2,var1,nvar)] > 0){
	    // Generate number of interactions;
	    ninteraction = gsl_ran_binomial(rng,interactions[IND(var2,var1,nvar)],npop);
	    for(interaction = 0; interaction < ninteraction; ++interaction){
	      // Generate interaction
	      //check to make sure targets is big enough...
	      index = pcg32_boundedrand(npop - interaction - 1) + 1;
	      if(targets[index] != person1){
		tmp = targets[interaction];
		targets[interaction] = targets[index];
		targets[index] = tmp;
	      }
	      if(possibleStates[var2][targets[interaction] ]){
		// fprintf(ifp,"%d:%d-%d:%d->%d\n",time,targets[interaction],person1,var2,var1);
		// printf("%d:%d-%d:%d->%d\n",time,targets[interaction],person1,var2,var1);
	        fwrite(&time,sizeof(time),1,ifp);
	        fwrite(&(targets[interaction]),sizeof(*targets),1,ifp);
	        fwrite(&person1,sizeof(person1),1,ifp);
	        fwrite(&var2,sizeof(var2),1,ifp);
	        fwrite(&var1,sizeof(var1),1,ifp);
		nextPossibleStates[var1][ targets[interaction] ] += 1;
	      }
	    }
          }
	}
      }
    }
  }
	  
  fclose(tfp);
  fclose(ifp);
  // free(actualTransitions);
  // free(actualInteractions);
  for(var1 = 0; var1 < nvar; ++ var1){
    free(possibleStates[var1]);
  }
  for(var1 = 0; var1 < nvar; ++ var1){
    free(nextPossibleStates[var1]);
  }
  free(possibleStates);
  free(nextPossibleStates);
  free(targets);
}

void runFullCounterfactualAnalysis(int* init,int nvar, int ntime, float* transitions, float* interactions, char* tfname, char* ifname, gsl_rng *rng){
  int npop,var1,var2,time,person1,person2,counter,tmp;
  double test;
  FILE *tfp;
  FILE *ifp;
  npop = 0;
  for(var1 = 0; var1< nvar; ++var1){
    npop = npop + init[var1];
  }
  int* actualTransitions;
  int* actualInteractions;
  int** possibleStates;
  int** nextPossibleStates;
  //Note: To help with memory useage, we may want to modify these to be more efficient
  // actualTransitions = malloc(nvar*nvar*ntime*npop*sizeof(int));
  // actualInteractions = malloc(nvar*nvar*ntime*npop*npop*sizeof(int));
  possibleStates = calloc(nvar,sizeof(int*));
  for(var1 = 0; var1 < nvar; ++ var1){
    possibleStates[var1] = calloc(npop,sizeof(int));
  }
  nextPossibleStates = malloc(nvar*sizeof(int*));
  for(var1 = 0; var1 < nvar; ++ var1){
    nextPossibleStates[var1] = calloc(npop,sizeof(int));
  }
  //Construct initial states:
  counter = 0;
  for(var1 = 0; var1 < nvar; ++ var1){
    for(person1 = counter; person1 < (counter + init[var1]); ++ person1){
      nextPossibleStates[var1][person1] += 1;
    }
    counter = counter + init[var1];
  }
  assert(counter == npop);

  // printf("Looping Variables:\n\tnvar is %d\n\tntime is %d\n\tnpop is %d\n",nvar,ntime,npop);
  tfp = fopen(tfname,"w");
  ifp = fopen(ifname,"w");

  for(time = 0; time < ntime; ++time){
    // printf("Time %d of %d\n",time,ntime);
    for(var1 = 0; var1 < nvar; ++var1){
      for(person1=0;person1<npop;++person1){
	possibleStates[var1][person1] = nextPossibleStates[var1][person1];
      }
    }
      
    for(person1=0;person1<npop;++person1){
      for(var1 = 0; var1 <nvar; ++var1){
	if(possibleStates[var1][person1] == 0) continue;
	for(var2 = 0; var2 <nvar; ++ var2){
	  if(transitions[IND(var1,var2,nvar)] > 0){
	    if(( (float) ldexp(pcg32_random(), -32)) < transitions[IND(var1,var2,nvar)]){
	      // printf("Here\n");
	      // fprintf(tfp,"%d,%d,%d,%d,%d\n",var1,var2,time,person1,actualTransitions[IND4(var1,var2,time,person1,nvar,ntime,npop)]);
	      fprintf(tfp,"%d:%d:%d->%d\n",time,person1,var1,var2);
	      nextPossibleStates[var2][person1] += 1;
	    }
          }
	  if(interactions[IND(var1,var2,nvar)] > 0){
	    for(person2=0;person2<npop;++person2){
	      if(possibleStates[var2][person2] == 0) continue;
	      // actualInteractions[IND5(var1,var2,time,person1,person2,nvar,ntime,npop,npop)] =
	      if(( (float) ldexp(pcg32_random(), -32)) < interactions[IND(var1,var2,nvar)]){
	        fprintf(ifp,"%d:%d-%d:%d->%d\n",time,person1,person2,var1,var2);
	        // fprintf(ifp,"%d,%d,%d,%d,%d,%d\n",time,var1,var2,person1,person2,actualInteractions[IND5(var1,var2,time,person1,person2,nvar,ntime,npop,npop)]);
	        nextPossibleStates[var2][person1] += 1;
	      }
	    }
          }
	}
      }
    }
  }
  for(person1 = 0;person1 < npop; ++person1){
    for(var1 = 0; var1 < nvar; ++var1){
      // printf("%d,",possibleStates[var1][person1]);
    }
    // printf("\n");
  }
	  
  fclose(tfp);
  fclose(ifp);
  // free(actualTransitions);
  // free(actualInteractions);
  for(var1 = 0; var1 < nvar; ++ var1){
    free(possibleStates[var1]);
  }
  for(var1 = 0; var1 < nvar; ++ var1){
    free(nextPossibleStates[var1]);
  }
  free(possibleStates);
  free(nextPossibleStates);
}

void constructTimeSeries(
  int* init,
  int nvar,
  int ntime,
  int (*reduceBeta)(int,int,int,int,int),
  void (*eliminateSusceptibles)(int**,int,int,int),
  char* tfname,
  char* ifname,
  char* outputfilename
){
  int var,person,time,reading,ttime,tperson,tvar1,tvar2,itime,iperson1,iperson2,ivar1,ivar2,reading_file_1,reading_file_2,npop,time2,err,counter,ctime,mtime;
  FILE *ofp;
  FILE *tfp;
  FILE *ifp;
  int** states;
  int* cur_states;
  int* state_counts;
  char test[1000];

  npop = 0;
  for(var = 0; var < nvar; ++ var){
    npop = npop + init[var];
  }
  state_counts = malloc(nvar*sizeof(int));
  states = malloc((1+ntime)*sizeof(int*));
  cur_states = calloc(npop,sizeof(int));
  for(time = 0; time < (ntime+1); ++ time){
    states[time] = calloc(npop,sizeof(int));
  }
  counter = 0;
  for(var = 0; var < nvar; ++ var){
    for(person = counter; person < counter+init[var];++person){
      // printf("person %d: %d\n",person,var);
      for(time = 0; time < (1+ntime); ++time){
	states[time][person] = var;
	cur_states[person] = var;
      }
    }
    counter = counter + init[var];
  }

  ofp = fopen(outputfilename,"w");
  tfp = fopen(tfname,"r");
  ifp = fopen(ifname,"r");

  reading=1;
  reading_file_1 = 1;
  reading_file_2 = 1;
  ctime = 0;
  itime = 0;
  ttime = 0;
  mtime = 0;
  while(reading == 1){
    // printf("%d|%d\n",itime,ttime);
    if(feof(tfp)){
      reading_file_1 = 0;
      if(feof(ifp)){
	reading = 0;
      }
    }
    if(feof(ifp)){
      reading_file_2 = 0;
    }
    if(
      (reading == 1) &&
      (reading_file_1 == 0) &&
      (reading_file_2 == 0) && 
      (ctime == mtime)
    ){
      fprintf(stderr,"This should not happen\n");
      exit(1);
    }
    // printf("1: r1 %d,r2 %d, t1 %d, t2 %d, t %d,tmax %d\n",reading_file_1,reading_file_2,ttime,itime,ctime,mtime);
    if(reading_file_1){
      // err = fscanf(tfp,"%d:%d:%d->%d\n",&ttime,&tperson,&tvar1,&tvar2);
      err = fread(&ttime,sizeof(int),1,tfp);
      err += fread(&tperson,sizeof(int),1,tfp);
      err += fread(&tvar1,sizeof(int),1,tfp);
      err += fread(&tvar2,sizeof(int),1,tfp);
      //Error checking
      // printf("%d:%d:%d->%d\n",ttime,tperson,tvar1,tvar2);
      if((err < 4) && (!feof(tfp))){
        fprintf(stderr,"Only caught %d params/4\n",err);
	exit(1);
      }
    }
    if(reading_file_2){
      //err = fscanf(ifp,"%d:%d-%d:%d->%d\n",&itime,&iperson1,&iperson2,&ivar1,&ivar2);
      err = fread(&itime,sizeof(int),1,ifp);
      err += fread(&iperson1,sizeof(int),1,ifp);
      err += fread(&iperson2,sizeof(int),1,ifp);
      err += fread(&ivar1,sizeof(int),1,ifp);
      err += fread(&ivar2,sizeof(int),1,ifp);
      //Error checking
      // printf("%d:%d-%d:%d->%d\n",itime,iperson1,iperson2,ivar1,ivar2);
      if((err < 5) && (!feof(tfp))){
        fprintf(stderr,"Only caught %d params/5\n",err);
	exit(1);
      }
      // printf("Matched %d/5\n",err);
    }
    reading_file_1 = ttime <= ctime ? 1 : 0 ;
    reading_file_2 = itime <= ctime ? 1 : 0 ;
    mtime = ((itime >= ttime) || feof(ifp)) ? ttime : itime;
    // printf("2: r1 %d,r2 %d, t1 %d, t2 %d, t %d,tmax %d\n",reading_file_1,reading_file_2,ttime,itime,ctime,mtime);
    if((reading_file_1 == 0) && (reading_file_2 == 0)){
      while(ctime < mtime){
	++ctime;
        for(person=0;person<npop;++person){
	  states[ctime][person] = cur_states[person];
        }
        // Printf("\t\t\t%d\n",ctime);
        // Beginning of time ctime
        eliminateSusceptibles(states,ctime,ntime,npop);
      }
    }
    reading_file_1 = ttime <= ctime ? 1 : 0 ;
    reading_file_2 = itime <= ctime ? 1 : 0 ;
    assert(ttime < ntime);
    assert(tperson < npop);
    assert(tvar1 < nvar);
    assert(tvar2 < nvar);
    assert(itime < ntime);
    assert(iperson1 < npop);
    assert(iperson2 < npop);
    assert(ivar1 < nvar);
    assert(ivar2 < nvar);

    //Figure out how to take into account precedence for these...
    //Order of operations may matter if transitions and/or interactions can move the same person to multiple categories...
    if(reading_file_1){
      if(states[ttime][tperson] == tvar1){
	cur_states[tperson] = tvar2;
/// 	for(time2 = ttime+1;time2 < (ntime+1);++time2){
///           //This seems like it could be problematic...
/// 	  states[time2][tperson] = tvar2;
/// 	}
      }
    }
    if(
      (states[itime][iperson1] == ivar1) &&
      (states[itime][iperson1] == states[itime+1][iperson1]) &&
      (states[itime][iperson2] == ivar2) &&
      (reduceBeta(itime,iperson1,iperson2,ivar1,ivar2) != 0) &&
      reading_file_2
    ){
      cur_states[iperson1] = ivar2;
    }
  }
  mtime = ntime;
  while(ctime < mtime){
    ++ctime;
    for(person=0;person<npop;++person){
      states[ctime][person] = cur_states[person];
    }
    eliminateSusceptibles(states,ctime,ntime,npop);
  }
///   for(person = 0; person < npop; ++person){
///     for(time = 0; time < (1+ntime); ++time){
///       fprintf(ofp,"%d,",states[time][person]);
///     }
///     fprintf(ofp,"\n");
///   }
  for(time = 0; time < (ntime+1); ++time){
    for(var = 0; var < nvar; ++ var){
      state_counts[var] = 0;
    }
    for(person = 0; person < npop; ++person){
      ++(state_counts[states[time][person] ]);
    }
    for(var = 0; var < nvar; ++var){
      fprintf(ofp,"%d,",state_counts[var]);
    }
    fprintf(ofp,"\n");
  }

  for(time = 0; time <(ntime + 1); ++time){
    free(states[time]);
  }
  free(states);
  free(cur_states);
  free(state_counts);
  fclose(ofp);
  fclose(tfp);
  fclose(ifp);
}

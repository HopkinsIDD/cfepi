						\
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <assert.h>
#include "pcg_variants.h"

#define IND(i,j,n) ((i) * (n) + (j))
#define IND4(i,j,k,l,n,o,p) ((i) * (n) * (o) * (p) + (j) * (o) * (p) + (k) * (p) + (l))
#define IND5(i,j,k,l,m,n,o,p,q) ((i) * (n) * (o) * (p) * (q) + (j) * (o) * (p) *(q) + (k) * (p) * (q) + (l) * (q) + (m))

void runCounterfactualAnalysis(char*,int*, int, int, float*, float*);
void runFullCounterfactualAnalysis(int*, int, int, float*, float*);
void constructTimeSeries(int* ,int, int,int (*reduceBeta)(int,int,int,int,int),void (*eliminateSusceptible)(int**, int,int,int),char*);



void runCounterfactualAnalysis(char* type, int* init,int nvar, int ntime, float* transitions, float* interactions){
  if(strcmp(type,"Full")==0){
    runFullCounterfactualAnalysis(init,nvar,ntime,transitions,interactions);
    return;
  }
  fprintf(stderr,"type %s is invalid\n",type);
  exit(1);
}

void runFullCounterfactualAnalysis(int* init,int nvar, int ntime, float* transitions, float* interactions){
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

  printf("Looping Variables:\n\tnvar is %d\n\tntime is %d\n\tnpop is %d\n",nvar,ntime,npop);
  tfp = fopen("transition.csv","w");
  ifp = fopen("interaction.csv","w");

  for(person1 = 0;person1 < npop; ++person1){
    // printf("%d:",person1);
    for(var1 = 0; var1 < nvar; ++var1){
      // printf("%d,",nextPossibleStates[var1][person1]);
    }
    // printf("\n");
  }

  for(time = 0; time < ntime; ++time){
    printf("Time %d of %d\n",time,ntime);
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
  free(possibleStates);
  for(var1 = 0; var1 < nvar; ++ var1){
    free(nextPossibleStates[var1]);
  }
  free(nextPossibleStates);
}

void constructTimeSeries(
  int* init,
  int nvar,
  int ntime,
  int (*reduceBeta)(int,int,int,int,int),
  void (*eliminateSusceptibles)(int**,int,int,int),
  char* outputfilename
){
  int var,person,time,reading,ttime,tperson,tvar1,tvar2,itime,iperson1,iperson2,ivar1,ivar2,reading_file_1,reading_file_2,npop,time2,err,counter,ctime;
  FILE *ofp;
  FILE *tfp;
  FILE *ifp;
  int** states;
  char test[1000];

  npop = 0;
  for(var = 0; var < nvar; ++ var){
    npop = npop + init[var];
  }
  states = malloc((1+ntime)*sizeof(int*));
  for(time = 0; time < (ntime+1); ++ time){
    states[time] = calloc(npop,sizeof(int));
  }
  counter = 0;
  for(var = 0; var < nvar; ++ var){
    for(person = counter; person < counter+init[var];++person){
      // printf("person %d: %d\n",person,var);
      for(time = 0; time < (1+ntime); ++time){
	states[time][person] = var;
      }
    }
    counter = counter + init[var];
  }
  for(person = 0; person < npop; ++person){
    for(time = 0; time < (1+ntime); ++time){
      // printf("%d,",states[time][person]);
    }
    // printf("\n");
  }
    

  ofp = fopen(outputfilename,"w");
  tfp = fopen("transition.csv","r");
  ifp = fopen("interaction.csv","r");

  reading=1;
  reading_file_1 = 1;
  reading_file_2 = 1;
  ctime = -1;
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
    if(reading_file_1){
      err = fscanf(tfp,"%d:%d:%d->%d\n",&ttime,&tperson,&tvar1,&tvar2);
      // printf("Matched %d/4\n",err);
      if(err < 4){
        fprintf(stderr,"Only caught %d params/4\n",err);
	exit(1);
      }
    }
    if(reading_file_2){
      err = fscanf(ifp,"%d:%d-%d:%d->%d\n",&itime,&iperson1,&iperson2,&ivar1,&ivar2);
      if(err < 5){
        fprintf(stderr,"Only caught %d params/5\n",err);
	exit(1);
      }
      // printf("Matched %d/5\n",err);
    }
    reading_file_1 = ttime <= itime ? 1 : 0 ;
    reading_file_2 = itime <= ttime ? 1 : 0 ;
    if((reading_file_1 == 1) && (ttime > ctime)){
      ctime = ttime;
			printf("\t\t\t%d\n",ctime);
      //Beginning of time ctime
      eliminateSusceptibles(states,ctime,ntime,npop);
    }
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
	for(time2 = ttime+1;time2 < (ntime+1);++time2){
	  states[time2][tperson] = tvar2;
	}
      }
    }
    if(reading_file_2 && (states[itime][iperson1] == states[itime+1][iperson1])){
      if((states[itime][iperson1] == ivar1) && (states[itime][iperson2] == ivar2)){
	if(reduceBeta(itime,iperson1,iperson2,ivar1,ivar2) != 0){
	  for(time2 = itime+1;time2 < (ntime+1);++time2){
	    states[time2][iperson1] = ivar2;
	  }
	}
      }
    }
  }
  for(person = 0; person < npop; ++person){
    for(time = 0; time < (1+ntime); ++time){
      fprintf(ofp,"%d,",states[time][person]);
    }
    fprintf(ofp,"\n");
  }

  for(time = 0; time <(ntime + 1); ++time){
    free(states[time]);
  }
  free(states);
  fclose(ofp);
  fclose(tfp);
  fclose(ifp);
}
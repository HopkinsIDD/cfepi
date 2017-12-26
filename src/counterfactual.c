#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "counterfactual.h"

// R specific headers
#include <R.h>
//#include <Rinternals.h>
#include <Rmath.h>
//#include <R_ext/Rdynload.h>
//#include <Rdefines.h>

// A utility function to swap to integers
void swap (int *a, int *b){
  int temp = *a;
  *a = *b;
  *b = temp;
}
/*
 * int* (*output) an integer vector to return with the sampes in it
 * int n          The original number to sample from
 * int k          The number of samples to draw
*/
void sample(int** output, int n, int k){
  int i,j;
  for (i = 0; i < k; i++){
    // Pick a random index from 0 to i
    j = runif(i,n+1);
    // printf("j is %d\n",j);
    if(j == i + 1){j = i;}
    // Swap arr[i] with the element at random index
    swap(&(*output)[i], &(*output)[j]);
  }
}

void runCounterfactualAnalysis(char* type, int* init,int nvar, int ntime, float* transitions, float* interactions,char* tfname, char* ifname){
  if(strcmp(type,"Full")==0){
    runFullCounterfactualAnalysis(init,nvar,ntime,transitions,interactions,tfname,ifname);
    return;
  }
  if(strcmp(type,"Fast")==0){
    runFastCounterfactualAnalysis(init,nvar,ntime,transitions,interactions,tfname,ifname);
    return;
  }
  fprintf(stderr,"type %s is invalid\n",type);
  return;
}

void runFastCounterfactualAnalysis(int* init,int nvar, int ntime, float* transitions, float* interactions,char* tfname,char* ifname){
  int npop,var1,var2,time,person1,counter,interaction;
  double ninteraction;
  FILE *tfp;
  FILE *ifp;
  FILE *tfp2;
  FILE *ifp2;
  char tfn2[1000];
  char ifn2[1000];
  npop = 0;
  for(var1 = 0; var1< nvar; ++var1){
    npop = npop + init[var1];
  }
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
  tfp = fopen(tfname,"wb");
  ifp = fopen(ifname,"wb");
  sprintf(ifn2,"%s.csv",ifname);
  sprintf(tfn2,"%s.csv",tfname);
  tfp2 = fopen(tfn2,"w");
  ifp2 = fopen(ifn2,"w");
  fprintf(ifp2,"Start\n");
  fflush(ifp2);

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
	    if(runif(0.0,1.0) < transitions[IND(var1,var2,nvar)]){
	      // fprintf(tfp,"%d,%d,%d,%d,%d\n",var1,var2,time,person1,actualTransitions[IND4(var1,var2,time,person1,nvar,ntime,npop)]);
	      // fprintf(tfp,"%d:%d:%d->%d\n",time,person1,var1,var2);
              // fprintf(tfp,"t:");
	      fprintf(tfp2,"%d:%d:%d->%d\n",time,person1,var1,var2);
	      // fprintf(tfp2,"%d:%d:%d->%d\n",sizeof(time),sizeof(person1),sizeof(var1),sizeof(var2));
	      fwrite(&time,sizeof(time),1,tfp);
	      fwrite(&person1,sizeof(person1),1,tfp);
	      fwrite(&var1,sizeof(var1),1,tfp);
	      fwrite(&var2,sizeof(var2),1,tfp);
	      nextPossibleStates[var2][person1] += 1;
	    }
          }
	  if(interactions[IND(var2,var1,nvar)] > 0){
	    // Generate number of interactions;
	    ninteraction = rbinom(npop,interactions[IND(var2,var1,nvar)]);
            // printf("There should be %f interactions\n",ninteraction);
	    // Generate interactions
            sample(&targets,npop,ninteraction);
            for(interaction = 0; interaction < ninteraction; ++interaction){
	      if(possibleStates[var2][targets[interaction] ]){
                fprintf(ifp2,"\t%d:%d-%d:%d->%d\n",time,targets[interaction],person1,var2,var1);
                // fprintf(ifp2,"\t%d:%d-%d:%d->%d\n",sizeof(time),sizeof(targets[interaction]),sizeof(person1),sizeof(var2),sizeof(var1) );
	        /*
	        fwrite(&time,sizeof(time),1,ifp);
	        fwrite(&(targets[interaction] ),sizeof(targets[interaction]),1,ifp);
	        fwrite(&person1,sizeof(person1),1,ifp);
	        fwrite(&var2,sizeof(var2),1,ifp);
	        fwrite(&var1,sizeof(var1),1,ifp);
	        */
	        fwrite(&time,sizeof(int),1,ifp);
	        fwrite(&(targets[interaction] ),sizeof(int),1,ifp);
	        fwrite(&person1,sizeof(int),1,ifp);
	        fwrite(&var2,sizeof(int),1,ifp);
	        fwrite(&var1,sizeof(int),1,ifp);
		nextPossibleStates[var1][ targets[interaction] ] += 1;
	      }
	    }
          }
	}
      }
    }
  }
	  
  fflush(ifp);
  fflush(tfp);
  fflush(ifp2);
  fflush(tfp2);
  fclose(tfp);
  fclose(ifp);
  fclose(tfp2);
  fclose(ifp2);
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

void runFullCounterfactualAnalysis(int* init,int nvar, int ntime, float* transitions, float* interactions, char* tfname, char* ifname){
  int npop,var1,var2,time,person1,person2,counter;
  FILE *tfp;
  FILE *ifp;
  npop = 0;
  for(var1 = 0; var1< nvar; ++var1){
    npop = npop + init[var1];
  }
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
	    if(runif(0.0,1.0) < transitions[IND(var1,var2,nvar)]){
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
	      if(runif(0.0,1.0) < interactions[IND(var1,var2,nvar)]){
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
	  
  fflush(ifp);
  fflush(tfp);
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
  int var,person,time,reading,ttime,tperson,tvar1,tvar2,itime,iperson1,iperson2,ivar1,ivar2,reading_file_1,reading_file_2,npop,err,counter,ctime,mtime;
  FILE *ofp;
  FILE *ofp2;
  FILE *tfp;
  FILE *ifp;
  int** states;
  int* cur_states;
  int* state_counts;
  char ofn2[1000];

  npop = 0;
  for(var = 0; var < nvar; ++ var){
    npop = npop + init[var];
  }
  sprintf(ofn2,"%s.csv",outputfilename);
  // ofp2 = fopen("output/test3.csv","w");
  ofp2 = fopen(ofn2,"w");
  state_counts = malloc(nvar*sizeof(int));
  states = malloc((1+ntime)*sizeof(int*));
  if(states == NULL){fprintf(stderr,"Malloc error for states\n");}
  cur_states = calloc(npop,sizeof(int));
  for(time = 0; time < (ntime+1); ++ time){
    states[time] = calloc(npop,sizeof(int));
  }
  counter = 0;
  for(var = 0; var < nvar; ++ var){
    for(person = counter; person < counter+init[var];++person){
      // fprintf(ofp2,"person %d: %d\n",person,var);
      for(time = 0; time < (1+ntime); ++time){
	states[time][person] = var;
	cur_states[person] = var;
      }
    }
    counter = counter + init[var];
  }

  ofp = fopen(outputfilename,"w");
  tfp = fopen(tfname,"rb");
  ifp = fopen(ifname,"rb");

  reading=1;
  reading_file_1 = 1;
  reading_file_2 = 1;
  ctime = 0;
  itime = 0;
  ttime = 0;
  mtime = 0;
  while(reading == 1){
    // fprintf(ofp2,"Loop\n");
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
      return;
    }
    fprintf(ofp2,"1: r1 %d,r2 %d, t1 %d, t2 %d, t %d,tmax %d\n",reading_file_1,reading_file_2,ttime,itime,ctime,mtime);
    if(reading_file_1){
      // err = fscanf(tfp,"%d:%d:%d->%d\n",&ttime,&tperson,&tvar1,&tvar2);
      err = fread(&ttime,sizeof(int),1,tfp);
      err += fread(&tperson,sizeof(int),1,tfp);
      err += fread(&tvar1,sizeof(int),1,tfp);
      err += fread(&tvar2,sizeof(int),1,tfp);
      //Error checking
      // printf("%d:%d:%d->%d\n",ttime,tperson,tvar1,tvar2);
      if((err < 4)){
        if(!feof(tfp)){
          fprintf(stderr,"Only caught %d params/4\n",err);
          return;
        } else {
          ttime=ntime-1;
          tperson=0;
          tvar1=0;
          tvar2=0;
        }
      } else {
        fprintf(ofp2,"%d:%d-%d->%d\n",ttime,tperson,tvar1,tvar2);
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
      if((err < 5)){
        if(!feof(ifp)){
          fprintf(stderr,"Only caught %d params/5\n",err);
          return;
        } else {
          iperson1=0;
          iperson2=0;
          ivar1=0;
          ivar2=0;
          itime=ntime-1;
        }
      } else {
        fprintf(ofp2,"%d:%d-%d->%d\n",itime,iperson1,ivar1,ivar2);
      }
      // printf("Matched %d/5\n",err);
    }
    reading_file_1 = ttime <= ctime ? 1 : 0 ;
    reading_file_2 = itime <= ctime ? 1 : 0 ;
    mtime = ((itime >= ttime) || feof(ifp)) ? ttime : itime;
    printf("2: r1 %d,r2 %d, t1 %d, t2 %d, t %d,tmax %d\n",reading_file_1,reading_file_2,ttime,itime,ctime,mtime);
    if((reading_file_1 == 0) && (reading_file_2 == 0)){
      while(ctime < mtime){
	++ctime;
        for(person=0;person<npop;++person){
	  states[ctime][person] = cur_states[person];
          fprintf(ofp2,"%d,",cur_states[person]);
        }
        fprintf(ofp2,"\n");
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
  fclose(ofp2);
  fclose(tfp);
  fclose(ifp);
}

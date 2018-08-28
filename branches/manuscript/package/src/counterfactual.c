#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "counterfactual.h"

// R specific headers
#include <R.h>
#include <R_ext/Print.h>
//#include <Rinternals.h>
#include <Rmath.h>
//#include <R_ext/Rdynload.h>
//#include <Rdefines.h>

/*
 * runCounterfacutalAnalysis
 *   Description
 *     Set up a counterfactual simulation.  This produces two files containing the set of possible spontaneous state transitions (transitions), and state transitions caused by interactions (interactions)
 *     See constructTimeSeries for how to use these files.
 *   Parameters
 *     string type Which type of counterfactual to run.  Current options are "Fast".  In the future, this option is reserved for different modeling assumptions.
 *     person_t vector init starting population in each state.
 *     time_t ntime The number of time steps to run the model over.
 *     transitions Matrix describing the probability of transitioning spontaneously between compartments
 *   
 */
void runCounterfactualAnalysis(char* type, person_t * init,var_t nvar, step_t ntime, double* transitions, double* interactions,char* tfname, char* ifname){
  if(strcmp(type,"Fast")==0){
    runFastCounterfactualAnalysis(init,nvar,ntime,transitions,interactions,tfname,ifname);
    return;
  }
  Rf_error("type %s is invalid\n",type);
  return;
}

/*
 * runFastCounterfactualAnalysis
 *   Description
 *     See runCounterfactualAnalysis for full description
 *     This makes the following assumptions - 
 *       No intervention increases the probability of an interaction occurring
 *       No intervention transitions people to states other than removed from the model.
 *       No intervention 
 *       
 */
void runFastCounterfactualAnalysis(person_t* init,var_t nvar, step_t ntime, double* transitions, double* interactions,char* tfname,char* ifname){
  var_t var1,var2;
  person_t person1, npop,interaction,counter;
  step_t time;
  double ninteraction;
  FILE *tfp;
  FILE *ifp;
  FILE *tfp2;
  FILE *ifp2;
  FILE *ofp2;
  char tfn2[1000];
  char ifn2[1000];
  char ofn2[1000];
  npop = 0;
  for(var1 = 0; var1< nvar; ++var1){
    npop = npop + init[var1];
  }
  if(CONSTRUCT_DEBUG){
    Rf_warning("npop is %d\n",npop);
  }
  bool_t** possibleStates;
  bool_t** nextPossibleStates;
  person_t* targets;
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
      nextPossibleStates[var1][person1] = 3;
    }
    counter = counter + init[var1];
  }
  assert(counter == npop);

  //Check to see if these exist
  if(CONSTRUCT_DEBUG==1){
    Rf_warning("Saving to files: %s and %s\n",tfname,ifname);
  }
  tfp = fopen(tfname,"wb");
  ifp = fopen(ifname,"wb");
  if(tfp == NULL){
    Rf_error("Could not open file %s\n",tfname);
    return;
  }
  if(ifp == NULL){
    Rf_error("Could not open file %s\n",ifname);
    return;
  }
  
  sprintf(ifn2,"%s.dat",ifname);
  sprintf(tfn2,"%s.dat",tfname);
  sprintf(ofn2,"%s.all.dat",ifname);
  if(CONSTRUCT_DEBUG==1){
    tfp2 = fopen(tfn2,"w");
    ifp2 = fopen(ifn2,"w");
    ofp2 = fopen(ofn2, "w");
    if(ofp2 == NULL){
      Rf_error("Could not open file %s.",ofn2);
    }
  }

  if(CONSTRUCT_DEBUG==1){
    Rf_warning("Writing each transition event will take %d + %d + %d + %d = %d\n",sizeof(step_t),sizeof(person_t),sizeof(var_t),sizeof(var_t),sizeof(step_t)+sizeof(person_t)+sizeof(var_t)+sizeof(var_t));
    Rf_warning("Writing each transition event will take %d + %d + %d + %d + %d = %d\n",sizeof(step_t),sizeof(person_t),sizeof(person_t),sizeof(var_t),sizeof(var_t),sizeof(step_t)+sizeof(person_t)+sizeof(person_t)+sizeof(var_t)+sizeof(var_t));
  }
  targets = malloc(npop * sizeof(person_t));
  for(person1 = 0; person1 < npop; ++person1){
    targets[person1] = person1;
  }

  for(time = 0; time < ntime; ++time){
    counter = 0;
    for(var1 = 0; var1 < nvar; ++var1){
      for(person1=0;person1<npop;++person1){
	possibleStates[var1][person1] = nextPossibleStates[var1][person1];
      }
      for(person1 = 0; person1 < counter; ++person1){
        nextPossibleStates[var1][person1] = MIN(nextPossibleStates[var1][person1],1);
      }
      for(person1 = counter; person1 < (counter + init[var1]); ++ person1){
        nextPossibleStates[var1][person1] = 2;
      }
      for(person1 = (counter + init[var1]); person1 < npop; ++person1){
        nextPossibleStates[var1][person1] = MIN(nextPossibleStates[var1][person1],1);
      }
      counter = counter + init[var1];
    }
      
    for(person1=0;person1<npop;++person1){
      for(var1 = 0; var1 <nvar; ++var1){
	if(possibleStates[var1][person1] == 0) continue;
	for(var2 = 0; var2 <nvar; ++ var2){
	  if(transitions[IND(var1,var2,nvar)] > 0){
	    if(runif(0.0,1.0) < transitions[IND(var1,var2,nvar)]){
	      if(CONSTRUCT_DEBUG==1){
                fprintf(tfp2,"%d:%d:%d->%d\n",time,person1,var1,var2);
                fprintf(ofp2,"%d:%d:%d->%d\n",time,person1,var1,var2);
              }
	      fwrite(&time,sizeof(step_t),1,tfp);
	      fwrite(&person1,sizeof(person_t),1,tfp);
	      fwrite(&var1,sizeof(var_t),1,tfp);
	      fwrite(&var2,sizeof(var_t),1,tfp);
	      nextPossibleStates[var2][person1] = 2;
	      nextPossibleStates[var1][person1] = nextPossibleStates[var1][person1] == 2 ? 2 : 0;
	    }
          }
	  if(interactions[IND(var2,var1,nvar)] > 0){
	    // Generate number of interactions;
	    ninteraction = rbinom(npop,interactions[IND(var2,var1,nvar)]);
            // Rf_warning("There should be %f interactions\n",ninteraction);
	    // Generate interactions
            /*
            if(ninteraction > 0){
              Rf_warning("\tnpop: %d\n\tninteraction: %f\n",npop,ninteraction);
            }
            */
            if(npop < ninteraction){
              Rf_error("This should not happen\n");
              return;
            }
            
            sample(&targets,npop,ninteraction);
            for(interaction = 0; interaction < ninteraction; ++interaction){
	      if(possibleStates[var2][targets[interaction] ]){
                if(CONSTRUCT_DEBUG==1){
                  fprintf(ifp2,"\t%d:%d-%d:%d->%d\n",time,targets[interaction],person1,var2,var1);
                  fprintf(ofp2,"\t%d:%d-%d:%d->%d\n",time,targets[interaction],person1,var2,var1);
                }
	        fwrite(&time,sizeof(step_t),1,ifp);
	        fwrite(&(targets[interaction] ),sizeof(person_t),1,ifp);
	        fwrite(&person1,sizeof(person_t),1,ifp);
	        fwrite(&var2,sizeof(var_t),1,ifp);
	        fwrite(&var1,sizeof(var_t),1,ifp);
		nextPossibleStates[var1][ targets[interaction] ] = 2;
		nextPossibleStates[var2][ targets[interaction] ] = nextPossibleStates[var1][targets[interaction]] == 2 ? 2 : 0;
	      }
	    }
          }
	}
      }
    }
    if(CONSTRUCT_DEBUG==1){
      for(var1 = 0; var1 < nvar; ++var1){
        for(person1 = 0 ; person1 < npop; ++person1){
          fprintf(ofp2,"%d ", nextPossibleStates[var1][person1]);
        }
        fprintf(ofp2,"\n");
      }
    }
  }
	  
  fflush(ifp);
  fflush(tfp);
  fclose(tfp);
  fclose(ifp);
  if(CONSTRUCT_DEBUG==1){
    fflush(ifp2);
    fflush(tfp2);
    fflush(ofp2);
    fclose(tfp2);
    fclose(ifp2);
    fclose(ofp2);
  }
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

void constructTimeSeries(
  person_t* init,
  var_t nvar,
  step_t ntime,
  saved_beta_t reduceBeta,
  saved_susceptible_t eliminateSusceptibles,
  char* tfname,
  char* ifname,
  char* outputfilename
){
  int err;
  var_t var, tvar1,tvar2,ivar1,ivar2;
  person_t person,tperson,iperson1,iperson2,npop,counter;
  step_t time, ttime, itime, mtime,ctime;
  bool_t reading,reading_file_1,reading_file_2,reading_tfp,reading_ifp;
  FILE *ofp;
  FILE *ofp2;
  char ofn2[1000];
  FILE *tfp;
  FILE *ifp;
  var_t** states;
  var_t* cur_states;
  person_t* state_counts;

  npop = 0;
  for(var = 0; var < nvar; ++ var){
    npop = npop + init[var];
  }
  if(RUN_DEBUG == 1){
    sprintf(ofn2,"%s.dat",outputfilename);
    ofp2 = fopen(ofn2,"w");
    fprintf(ofp2,"npop is %d\n",npop);
  }
  state_counts = malloc(nvar*sizeof(person_t));
  states = malloc((1+ntime)*sizeof(var_t*));
  if(states == NULL){Rf_error("Malloc error for states\n");}
  cur_states = calloc(npop,sizeof(var_t));
  for(time = 0; time < (ntime+1); ++ time){
    states[time] = calloc(npop,sizeof(var_t));
  }
  counter = 0;
  for(var = 0; var < nvar; ++ var){
    for(person = counter; person < counter+init[var];++person){
      for(time = 0; time < (1+ntime); ++time){
	states[time][person] = var;
	cur_states[person] = var;
      }
    }
    counter = counter + init[var];
  }

  ofp = fopen(outputfilename,"w");
  if(ofp == NULL){
    Rf_error("Could not open file %s.",outputfilename);
  }
  tfp = fopen(tfname,"rb");
  if(tfp == NULL){
    Rf_error("Could not open file %s.",tfname);
  }
  reading_tfp = tfp == NULL ? 0 : 1;
  ifp = fopen(ifname,"rb");
  if(ifp == NULL){
    Rf_error("Could not open file %s.",ifname);
  }
  reading_ifp = tfp == NULL ? 0 : 1;

  reading= MAX(reading_tfp,reading_ifp);
  reading_file_1 = reading_tfp;
  reading_file_2 = reading_ifp;
  ctime = 0;
  itime = 0;
  ttime = 0;
  mtime = 0;
  while(reading == 1){
    //Check to see if files are ended
    if(RUN_DEBUG==1){
      fprintf(ofp2,"Loop\n");
    }
    // Rf_warning("transition reading %p - %d\n",tfp,reading_tfp);
    if((reading_tfp) && (feof(tfp))){
      reading_file_1 = 0;
      if((reading_ifp) && (feof(ifp))){
	reading = 0;
      }
    }
    if((reading_ifp) && (feof(ifp))){
      reading_file_2 = 0;
    }
    if(
      (reading == 1) &&
      (reading_file_1 == 0) &&
      (reading_file_2 == 0) && 
      (ctime == mtime)
    ){
      Rf_error("This should not happen\n");
      return;
    }
    if(RUN_DEBUG==1){
      fprintf(ofp2,"1: r1 %d,r2 %d, t1 %d, t2 %d, t %d,tmax %d\n",reading_file_1,reading_file_2,ttime,itime,ctime,mtime);
    }
    // Read transitions and edges from files that we are currently reading

    if(reading_file_1){
      err = fread(&ttime,sizeof(step_t),1,tfp);
      err += fread(&tperson,sizeof(person_t),1,tfp);
      err += fread(&tvar1,sizeof(var_t),1,tfp);
      err += fread(&tvar2,sizeof(var_t),1,tfp);
      //Error checking
      if((err < 4)){
        if(!feof(tfp)){
          Rf_error("Only caught %d params/4\n",err);
          return;
        } else {
          ttime=ntime-1;
          tperson=0;
          tvar1=0;
          tvar2=0;
        }
      } else {
        if(RUN_DEBUG==1){
          fprintf(ofp2,"%d:%d:%d->%d\n",ttime,tperson,tvar1,tvar2);
        }
      }
    }
    if(reading_file_2){
      //err = fscanf(ifp,"%d:%d-%d:%d->%d\n",&itime,&iperson1,&iperson2,&ivar1,&ivar2);
      err = fread(&itime,sizeof(step_t),1,ifp);
      err += fread(&iperson1,sizeof(person_t),1,ifp);
      err += fread(&iperson2,sizeof(person_t),1,ifp);
      err += fread(&ivar1,sizeof(var_t),1,ifp);
      err += fread(&ivar2,sizeof(var_t),1,ifp);
      //Error checking
      // Rf_warning("%d:%d-%d:%d->%d\n",itime,iperson1,iperson2,ivar1,ivar2);
      if((err < 5)){
        if(!feof(ifp)){
          Rf_error("Only caught %d params/5\n",err);
          return;
        } else {
          iperson1=0;
          iperson2=0;
          ivar1=0;
          ivar2=0;
          itime=ntime-1;
        }
      } else {
        if(RUN_DEBUG==1){
          fprintf(ofp2,"%d:%d-%d:%d->%d\n",itime,iperson1,iperson2,ivar1,ivar2);
        }
      }
    }

    // Check to see if we should read the next line of each file
    reading_file_1 = ttime <= ctime ? reading_tfp : 0 ;
    reading_file_2 = itime <= ctime ? reading_ifp : 0 ;
    mtime = ((itime >= ttime) || feof(ifp)) ? ttime : itime;
    if(RUN_DEBUG==1){
      fprintf(ofp2,"2: r1 %d,r2 %d, t1 %d, t2 %d, t %d,tmax %d\n",reading_file_1,reading_file_2,ttime,itime,ctime,mtime);
    }
    // Advance time if there is nothing left to read
    if((reading_file_1 == 0) && (reading_file_2 == 0)){
        if(RUN_DEBUG == 1){
          fprintf(ofp2,"time %d\n",ctime);
        }
	++ctime;
        for(person=0;person<npop;++person){
	  states[ctime][person] = cur_states[person];
        }
        // Printf("\t\t\t%d\n",ctime);
        // Beginning of time ctime
        invoke_susceptible_t(eliminateSusceptibles,states,ctime,ntime,npop);
        for(person=0;person<npop;++person){
          cur_states[person] = states[ctime][person];
          if(RUN_DEBUG==1){
            fprintf(ofp2,"%d,",cur_states[person]);
          }
        }
        if(RUN_DEBUG==1){
          fprintf(ofp2,"\n");
      }
    }

    // check to see if we should keep reading now that time has advanced
    reading_file_1 = ttime <= ctime ? reading_tfp : 0 ;
    reading_file_2 = itime <= ctime ? reading_ifp : 0 ;
    // Check that we aren't in a bad place
    assert(ttime < ntime);
    assert(tperson < npop);
    assert(tvar1 < nvar);
    assert(tvar2 < nvar);
    assert(itime < ntime);
    assert(iperson1 < npop);
    assert(iperson2 < npop);
    assert(ivar1 < nvar);
    assert(ivar2 < nvar);
    assert(ttime >= ctime);
    assert(itime >= ctime);

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
    // Rf_warning("reduceBeta: %p\nitime %d\niperson1 %d\niperson2 %d\nivar1 %d\nivar2 %d\n\n", &reduceBeta,itime,iperson1,iperson2,ivar1,ivar2);
    if(
      (states[itime][iperson1] == ivar1) &&
      // (states[itime][iperson1] == states[itime+1][iperson1]) &&
      (states[itime][iperson2] == ivar2) &&
      (invoke_beta_t(reduceBeta,itime,iperson1,iperson2,ivar1,ivar2) != 0) &&
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
    invoke_susceptible_t(eliminateSusceptibles,states,ctime,ntime,npop);
    for(person=0;person<npop;++person){
      cur_states[person] = states[ctime][person];
    }
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
  if(RUN_DEBUG == 1){
    fclose(ofp2);
  }
  fclose(ofp);
  if(reading_tfp){
    fclose(tfp);
  }
  if(reading_ifp){
    fclose(ifp);
  }
}

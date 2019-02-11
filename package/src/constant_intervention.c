#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "constant_intervention.h"

#include <R.h>
#include <R_ext/Print.h>
#include <Rmath.h>

param_susceptible_t param_constant_susceptible(step_t time, float coverage, person_t npop, float rate,var_t move_to, var_t move_from,var_t nfrom){
  person_t i;
  // if(RUN_DEBUG){
  //   Rf_warning("Creating param_constant_susceptible from time %d, coverage %f, population %d, rate %f, to %d, from %d, nfrom %d",time,coverage,npop,rate,move_to,move_from);
  // }
  param_susceptible_t rc;
  strcpy(rc.type,"constant");
  rc.data = malloc(1 * sizeof(data_susceptible_constant_t));
  (* ( (data_susceptible_constant_t *) rc.data) ).time = time;
  (* ( (data_susceptible_constant_t *) rc.data) ).rate = rate;
  (* ( (data_susceptible_constant_t *) rc.data) ).coverage = coverage;
  (* ( (data_susceptible_constant_t *) rc.data) ).move_to = move_to;
  (* ( (data_susceptible_constant_t *) rc.data) ).nfrom = nfrom;
  if(nfrom != 1){
    Rf_error("We currently only support a single origin compartment.");
  }
  // (* ( (data_susceptible_constant_t *) rc.data) ).move_from = malloc((* ( (data_susceptible_constant_t *) rc.data) ).nfrom * sizeof(var_t));
  (* ( (data_susceptible_constant_t *) rc.data) ).move_from = move_from;
  
  (* ( (data_susceptible_constant_t *) rc.data) ).naffected = rbinom(npop,coverage);
  (* ( (data_susceptible_constant_t *) rc.data) ).targets = malloc(npop*sizeof(person_t));
  for(i = 0 ; i < npop; ++ i){
    (* ( (data_susceptible_constant_t *) rc.data) ).targets[i] = i;
  }
  // if(RUN_DEBUG){
  //   Rf_warning("Sampling %d of %d\n",(* ( (data_susceptible_constant_t *) rc.data) ).naffected,npop);
  // }
  sample(&((* ( (data_susceptible_constant_t *) rc.data) ).targets),npop,(* ( (data_susceptible_constant_t *) rc.data) ).naffected);
  return(rc);
}

bool_t free_param_constant_susceptible(param_susceptible_t rc){
  if(strcmp(rc.type,"constant")!=0){
    Rf_error("Attempting to free a param_susceptible_t with the wrong destructor\n");
    return(1);
  }
  
  if((* ( (data_susceptible_constant_t *) rc.data) ).nfrom > 0){
    //free((* ( (data_susceptible_constant_t *) rc.data) ).move_from);
  }
  free((* ( (data_susceptible_constant_t *) rc.data) ).targets);
  free(rc.data);
  return(0);
}

// Finish writing these
void constant_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  person_t nmoved, person1;
  int counter,second_counter,third_counter;
  data_susceptible_constant_t pardata;
  var_t var1;
  time_t time1;
  if(time == (* ( (data_susceptible_constant_t *) pars.data) ).time){
    pardata = *( (data_susceptible_constant_t *) pars.data);
    (*( (data_susceptible_constant_t *) pars.data)).naffected = rbinom(npop,(*( (data_susceptible_constant_t *) pars.data)).coverage);
    Rf_warning("I chose %d people to take antivirals",(*( (data_susceptible_constant_t *) pars.data)).naffected);
    sample(&((*( (data_susceptible_constant_t *) pars.data)).targets),npop,(*( (data_susceptible_constant_t *) pars.data)).naffected);
    // Rf_warning("Those people were:");
    // for(nmoved = 0; nmoved < (*( (data_susceptible_constant_t *) pars.data)).naffected; ++nmoved){
    //   Rf_warning("\t%d",((*( (data_susceptible_constant_t *) pars.data)).targets)[nmoved]);
    // }
  }
  if(time >= (* ( (data_susceptible_constant_t *) pars.data) ).time){
    // Determine number affected accordin to binomial distribution
    pardata = *( (data_susceptible_constant_t *) pars.data);
    nmoved = rbinom(pardata.naffected,pardata.rate);
    sample(&(pardata.targets),pardata.naffected,nmoved);
    counter = 0;
    third_counter = 0;
    for(person1 = 0; person1 < nmoved; ++person1){
      ++third_counter;
      second_counter = 0;
      //Vaccinate targets[person1]
      for(var1 = 0; var1 < pardata.nfrom; ++ var1){
        ++second_counter;
        if(states[time-1][(pardata.targets)[person1]] == pardata.move_from){
          // if(RUN_DEBUG==1){
          //   Rf_warning("Moved person %d at time %d from state %d to %d\n",pardata.targets[person1],time,pardata.move_from,pardata.move_to);
          // }
          ++counter;
          for(time1 = time; time1 < (ntime+1); ++time1){
            states[time1][(pardata.targets)[person1]] = pardata.move_to;
          }
        }
      }
    }
    // Rf_warning("I chose %d people to take antivirals, and %f would have recovered because of them, but only %f actually did. Also note %d ol and %d il",pardata.naffected,nmoved*1./pardata.naffected,counter*1.0/pardata.naffected,second_counter,third_counter);
  }
}

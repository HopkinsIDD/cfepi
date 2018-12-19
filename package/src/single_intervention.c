#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "single_intervention.h"

#include <R.h>
#include <R_ext/Print.h>
#include <Rmath.h>

param_susceptible_t param_single_susceptible(step_t time, float rate,var_t move_to, var_t move_from,var_t nfrom){
  param_susceptible_t rc;
  strcpy(rc.type,"single");
  rc.data = malloc(1 * sizeof(data_susceptible_single_t));
  (* ( (data_susceptible_single_t *) rc.data) ).time = time;
  (* ( (data_susceptible_single_t *) rc.data) ).rate = rate;
  (* ( (data_susceptible_single_t *) rc.data) ).move_to = move_to;
  (* ( (data_susceptible_single_t *) rc.data) ).nfrom = nfrom;
  // (* ( (data_susceptible_single_t *) rc.data) ).move_from = malloc((* ( (data_susceptible_single_t *) rc.data) ).nfrom * sizeof(var_t));
  (* ( (data_susceptible_single_t *) rc.data) ).move_from = move_from;
  return(rc);
}

bool_t free_param_single_susceptible(param_susceptible_t rc){
  if(strcmp(rc.type,"single")!=0){
    Rf_error("Attempting to free a param_susceptible_t with the wrong destructor\n");
    return(1);
  }
  
  if((* ( (data_susceptible_single_t *) rc.data) ).nfrom > 0){
    //free((* ( (data_susceptible_single_t *) rc.data) ).move_from);
  }
  free(rc.data);
  return(0);
}

// Finish writing these
void single_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){
  person_t nmoved, person1;
  data_susceptible_single_t pardata;
  person_t* targets;
  var_t var1;
  time_t time1;
  if(time == (* ( (data_susceptible_single_t *) pars.data) ).time){
    // Vaccinate according to rbinom
    pardata = *( (data_susceptible_single_t *) pars.data);
    // For debugging
    nmoved = rbinom(npop,pardata.rate);
    targets = malloc(npop * sizeof(person_t));
    for(person1 = 0; person1 < npop; ++person1){
      targets[person1] = person1;
    }
    sample(&targets,npop,nmoved);
    for(person1 = 0; person1 < nmoved; ++person1){
      //Vaccinate targets[person1]
      for(var1 = 0; var1 < pardata.nfrom; ++ var1){
	if(states[time][targets[person1]] == pardata.move_from){
          for(time1 = time; time1 < (ntime+1); ++time1){
	    states[time1][targets[person1]] = pardata.move_to;
          }
	}
      }
    }
    free(targets);
  }
}

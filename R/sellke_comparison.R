library(reshape2)
library(dplyr)
library(animation)

set.seed(1234)
ntrial = 100
results = 1:ntrial * NA
final_size = 1:ntrial * NA

gridnum = 5
gridsize = 2*gridnum + 1
tmax = 500
beta = 4/(3*3 - 1)# 8 is the window size
gamma = .1
vacc_delay = 1

###     if(trial == 7){
###       if(t %in% c(6,7)){
###         browser()
###       }
###     }

run_simulation_mine = function(i_event,r_event,t,states){
  tracker[tracker[] < 0] <<- 0
  # Update exposure/recovery clocks
  for(row in 1:gridsize){
    for(col in 1:gridsize){
      if(states[row,col,t-1] == 1){
        ## Recovery clocks
        tracker[row,col] <<- tracker[row,col] + 1

        ## Exposure Clocks
        rmin = max(row - 1,1)
        rmax = min(row + 1,gridsize)
        cmin = max(col - 1,1)
        cmax = min(col + 1,gridsize)
        for(nrow in rmin:rmax){
          for(ncol in cmin:cmax){
            if(states[nrow,ncol,t-1] == 0){
              tracker[nrow,ncol] <<- tracker[nrow,ncol] + 1
            }
          }
        }
      }
    }
  }
  for(row in 1:gridsize){
    for(col in 1:gridsize){
      if(states[row,col,t-1] == 1){
        if(r_event[row,col,t-1] > 0){
          states[row,col,t] = 2
          r_thresh[row,col] <<- tracker[row,col]
          tracker[row,col] <<- -1
        }
        
        ## Check for new infections
        rmin = max(row - 1,1)
        rmax = min(row + 1,gridsize)
        cmin = max(col - 1,1)
        cmax = min(col + 1,gridsize)
        for(nrow in rmin:rmax){
          for(ncol in cmin:cmax){
            if((tracker[nrow,ncol] >= 0) & (i_event[row,col,nrow,ncol,t-1] > 0) & (states[row,col,t-1] == 1) & (states[nrow,ncol,t-1] == 0)){
              states[nrow,ncol,t] = 1
              i_thresh[nrow,ncol] <<- tracker[nrow,ncol]
              tracker[nrow,ncol] <<- -1
            }
          }
        }
      }
    }
  }
  tmp_old = states[,,t-1]
  tmp_new = states[,,t]
  tmp_new[is.na(tmp_new)] = tmp_old[is.na(tmp_new)]
  states[,,t] = tmp_new
  return(states)
}

run_simulation_sellke = function(i_thresh,r_thresh,t,states){
  sellke_tracker[sellke_tracker[] < 0] <<- 0
  # Update exposure/recovery clocks
  for(row in 1:gridsize){
    for(col in 1:gridsize){
      if(states[row,col,t-1] == 1){
        ## Recovery clocks
        sellke_tracker[row,col] <<- sellke_tracker[row,col] + 1

        ## Exposure Clocks
        rmin = max(row - 1,1)
        rmax = min(row + 1,gridsize)
        cmin = max(col - 1,1)
        cmax = min(col + 1,gridsize)
        for(nrow in rmin:rmax){
          for(ncol in cmin:cmax){
            if(states[nrow,ncol,t-1] == 0){
              sellke_tracker[nrow,ncol] <<- sellke_tracker[nrow,ncol] + 1
            }
          }
        }
      }
    }
  }
  for(row in 1:gridsize){
    for(col in 1:gridsize){
      if(states[row,col,t-1] == 1){
        if(sellke_tracker[row,col] >= r_thresh[row,col]){
          sellke_tracker[row,col] <<- -1
          states[row,col,t] = 2
        }
        ## Check for new infections
        rmin = max(row - 1,1)
        rmax = min(row + 1,gridsize)
        cmin = max(col - 1,1)
        cmax = min(col + 1,gridsize)
        for(nrow in rmin:rmax){
          for(ncol in cmin:cmax){
            if((sellke_tracker[nrow,ncol] >= i_thresh[nrow,ncol]) & (states[nrow,ncol,t-1] == 0)){
              sellke_tracker[nrow,ncol] <<- -1
              states[nrow,ncol,t] = 1
            }
          }
        }
      }
    }
  }
  tmp_old = states[,,t-1]
  tmp_new = states[,,t]
  tmp_new[is.na(tmp_new)] = tmp_old[is.na(tmp_new)]
  states[,,t] = tmp_new
  return(states)
}

run_simulation = function(i_event,r_event,tmax,intervention=TRUE,type='event'){
  states = array(NA,c(gridsize,gridsize,tmax))
  states[,,1] = 0
  states[gridnum + 1,gridnum + 1,1] = 1
  sellke_tracker <<- tracker
  sellke_tracker[] <<- 0
  for(t in 2:tmax){
    if(type == 'mine'){
      states = run_simulation_mine(i_event,r_event,t,states)
    } else if(type == 'sellke'){
      states = run_simulation_sellke(i_thresh,r_thresh,t,states)
    } else {
      stop("No such type")
    }
    ## Intervention
    if(intervention){
      for(row in 1:gridsize){
        for(col in 1:gridsize){
          # intervention change states
          if(t > vacc_delay){
            if((states[row,col,t-vacc_delay] == 1)){
              rmin = max(row - 1,1)
              rmax = min(row + 1,gridsize)
              cmin = max(col - 1,1)
              cmax = min(col + 1,gridsize)
              for(nrow in rmin:rmax){
                for(ncol in cmin:cmax){
                  if(states[nrow,ncol,t] == 0){
                    states[nrow,ncol,t] = 3
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return(states)
}

all_results = array(NA,c(2,2,gridsize,gridsize,tmax,ntrial))
dimnames(all_results)[[1]] <- c('no intervention','intervention')
dimnames(all_results)[[2]] <- c('mine','sellke')

for(trial in 1:ntrial){
  print(trial)
  intervention_mine= array(NA,c(gridsize,gridsize,tmax))
  intervention_sellke = array(NA,c(gridsize,gridsize,tmax))
  r_thresh = i_thresh = sellke_tracker = tracker = matrix(NA,gridsize,gridsize)
  tracker[] = 0
  sellke_tracker[] = 0
  intervention_sellke[,,1] = 0
  intervention_sellke[gridnum + 1,gridnum + 1,1] = 1
  intervention_mine[,,1] = 0
  intervention_mine[gridnum + 1,gridnum + 1,1] = 1
  i_event = array(NA,c(gridsize,gridsize,gridsize,gridsize,tmax))
  i_event[] = rbinom(prod(dim(i_event)),1,beta)
  r_event = array(NA,c(gridsize,gridsize,tmax))
  r_event[] = rbinom(prod(dim(r_event)),1,gamma)
  
  no_intervention_mine = run_simulation(i_event,r_event,tmax,FALSE,'mine')
  # if(trial == 7){browser()}
  r_thresh[is.na(r_thresh)] = tracker[is.na(r_thresh)] + 1
  i_thresh[is.na(i_thresh)] = tracker[is.na(i_thresh)] + 1
  first_i_thresh = i_thresh
  first_r_thresh = r_thresh

  no_intervention_sellke = run_simulation(first_i_thresh,first_r_thresh,tmax,FALSE,'sellke')
  intervention_sellke = run_simulation(first_i_thresh,first_r_thresh,tmax,TRUE,'sellke')

  no_intervention_mine = run_simulation(i_event,r_event,tmax,FALSE,'mine')
  intervention_mine = run_simulation(i_event,r_event,tmax,TRUE,'mine')

  tmp1 = intervention_mine
  tmp1[tmp1==3] = 0
  tmp2 = intervention_sellke
  tmp2[tmp2==3] = 0
  raw_difference = (intervention_mine != intervention_sellke)[,,tmax]
  raw_difference
  results[trial] = sum(raw_difference)/gridsize/gridsize
  net_difference = sum(intervention_mine[,,tmax] > 0) - sum(intervention_sellke[,,tmax] > 0)
  final_size[trial] = net_difference/gridsize/gridsize
  
  all_results[1,1,,,,trial] = no_intervention_mine
  all_results[1,2,,,,trial] = no_intervention_sellke
  all_results[2,1,,,,trial] = intervention_mine
  all_results[2,2,,,,trial] = intervention_sellke
  
  no_intervention_comp = (no_intervention_mine - no_intervention_sellke)
  if(!isTRUE(all.equal(no_intervention_mine,no_intervention_sellke))){
    print(paste("Divergence at time",min(which(no_intervention_comp != 0,arr.ind= T)[,3])))
    stop("Algorithms diverged in the non intervention case")
  }
}
all_results[all_results == 3] = 0
intervention_effect_mine = all_results[2,1,,,,] - all_results[1,1,,,,]
intervention_effect_sellke = all_results[2,1,,,,] - all_results[1,1,,,,]

all_results = as_data_frame(melt(all_results))
names(all_results) = c('intervention','algorithm','row','col','time','trial','state')

all_results %>%
  group_by(state,algorithm,intervention) %>%
  summarize(num = length(time)) %>%
  spread(algorithm,num)

max_relevent_time = all_results %>% filter(state == 1) %>% summarize(time = max(time)) %>% .$time

oopt <- ani.options(interval = .1, nmax = max_relevent_time)
for(i in 1:ani.options("nmax")){
  plt = all_results %>%
    filter(time == i) %>%
    group_by(row,col,intervention,algorithm) %>%
    summarize(state = mean(state >= 1)) %>%
    ggplot() +
    geom_tile(aes(x=row,y=col,fill=state)) +
    facet_grid(intervention~algorithm)
  print(plt)
  ani.pause()
}

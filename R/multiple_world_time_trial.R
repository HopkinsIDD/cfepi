source("R/intervention_params.R")
run_mw_inference <- function(ntime,npop,ninf,ntrial,R0=1.75,modified_R0=R0,D=2.25,vcr=0,delta_D=0,start_time=1){
  print(paste(ntime,npop,ninf,ntrial,R0,modified_R0,D,vcr,delta_D,start_time))
  nvar = 6
  all_results = array(NA,c(ntrial,ntime,nvar))
  dimnames(all_results) <- list(
    trial = 1:ntrial,
    time = 1:ntime,
    var = c("S","I","R","V","S_a","I_a")
  )
  times = 1:ntime * NA
  for(trial in 1:ntrial){
    initial = c(npop-ninf,ninf,0)
    initial[4:6] = 0
    ## Antivirals
    if(delta_D > 0){
      nantiviral1 = rbinom(1,initial[1],prop)
      initial[5] = nantiviral1
      initial[1] = initial[1] - nantiviral1
      nantiviral2 = rbinom(1,initial[2],prop)
      initial[6] = nantiviral2
      initial[2] = initial[2] - nantiviral2
    }
    N = sum(initial)
    current = initial
    gamma = 1/D
    beta = R0 * gamma/npop
    modified_beta = modified_R0 * gamma/npop;
    gamma_av = 1/(D-delta_D)
    Rprof()
    for(i in 1:ntime){
      if((i == start_time) & (vcr > 0)){
        nvaccinated = rbinom(1,current[1],vcr)
        current[4] = nvaccinated
        current[1] = current[1] - nvaccinated
      }
      all_results[trial,i,] = current
      # Infections
      if(i >= start_time){
        effective_beta = 1 - (1- modified_beta)^(current[2] + current[6])
      } else {
        effective_beta = 1 - (1- beta)^(current[2] + current[6])
      }
      ninfected = rbinom(n=1,size=current[1],prob=effective_beta)
      nrecovered = rbinom(n=1,size=current[2],prob=1/D)
      current[1] = current[1] - ninfected
      current[2] = current[2] + ninfected - nrecovered
      current[3] = current[3] + nrecovered
      if((delta_D > 0)){
        ninfected = rbinom(n=1,size=current[5],prob=effective_beta)
        if((i > start_time)){
          nrecovered = rbinom(n=1,size=current[6],prob=(gamma_av))
        } else {
          nrecovered = rbinom(n=1,size=current[6],prob=(gamma))
        }
        current[5] = current[5] - ninfected
        current[6] = current[6] + ninfected - nrecovered
        current[3] = current[3] + nrecovered
      }
    }
    Rprof(NULL)
    tmp = summaryRprof()
    times[trial] = tmp$sampling.time
  }
  print(mean(times))
  return(all_results)
}

library(abind)
# hand_washing_pars = list(start_time = tstar, rate= 1-sigma)
# vaccination_pars = list(intervention_time = tstar, rate = vaccination_rate, to=3,from=0)
# antiviral_pars = list(intervention_time = tstar, coverage = prop, rate = antiviral_rate, to=2, from=1)
# hw_name = paste("Hand Washing",paste(gsub('.','',unlist(hand_washing_pars),fixed=T),collapse='-'),sep='-')
# va_name = paste("Vaccination",paste(gsub('.','',unlist(vaccination_pars),fixed=T),collapse='-'),sep='-')
# av_name = paste("Antivirals",paste(gsub('.','',unlist(antiviral_pars),fixed=T),collapse='-'),sep='-')
# ni_name = "No Intervention"

run_all_mw_inference = function(ntime,npop,ninf,ntrial,start_time,gamma){
  rc <- abind(
    run_mw_inference(ntime,npop,ninf,ntrial,R0,R0*sigma,start_time=tstar,D=1/gamma),
    run_mw_inference(ntime,npop,ninf,ntrial,R0,R0,vcr = vaccination_rate,start_time=start_time,D=1/gamma),
    run_mw_inference(ntime,npop,ninf,ntrial,R0,R0,delta_D = sick_reduction,start_time=start_time,D=1/gamma),
    run_mw_inference(ntime,npop,ninf,ntrial,R0,R0,D=1/gamma),
    run_mw_inference(ntime,npop,ninf,ntrial,R0,R0,D=1/gamma),
    along=4)
  dimnames(rc)[[4]] = c(
    hw_name,
    va_name,
    av_name,
    ni_name,
    "Uncontrolled"
  )
  return(rc)
}
library(reshape2)
library(tidyr)
mw_output = melt(run_all_mw_inference(ntime,sum(init),init[2],ntrial,tstar,gamma)) %>% as_data_frame()
names(mw_output) = c("trial",'t','variable','intervention','value')
mw_output$type = 'Traditional'
mw_output = spread(mw_output,variable,value)
mw_output$S = mw_output$S + mw_output$S_a
mw_output$I = mw_output$I + mw_output$I_a
mw_output$S_a = NULL
mw_output$I_a = NULL
mw_output = gather(mw_output,variable,value,S,I,R,V)
mw_output$scenario = paste('main',paste(args[1:5],collapse='-'),sep='')
mw_residuals = spread(mw_output,intervention,value)
mw_residuals[[hw_name]] = mw_residuals[[hw_name]] - mw_residuals$Uncontrolled
mw_residuals[[va_name]] = mw_residuals[[va_name]] - mw_residuals$Uncontrolled
mw_residuals[[av_name]] = mw_residuals[[av_name]] - mw_residuals$Uncontrolled
mw_residuals[[ni_name]] = mw_residuals[[ni_name]] - mw_residuals$Uncontrolled
mw_residuals = gather_(mw_residuals,'intervention','value',c(hw_name,av_name,ni_name,va_name))
mw_residuals$scenario = paste('main',paste(args[1:5],collapse='-'),sep='')
mw_output$Uncontrolled=NULL
mw_residuals$Uncontrolled =NULL

source("R/intervention_params.R")
run_mw_inference <- function(ntime,npop,ninf,ntrial,R0=1.75,D=2.25,vcr=0,delta_D=0){
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
    ## Vaccination
    if(vcr > 0){
      nvaccinated = rbinom(1,initial[1],vcr)
      initial[4] = nvaccinated
      initial[1] = initial[1] - nvaccinated
    }
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
    Rprof()
    for(i in 1:ntime){
      all_results[trial,i,] = current
      # Infections
      effective_beta = 1 - (1- beta)^(current[2] + current[6])
      ninfected = rbinom(n=1,size=current[1],prob=effective_beta)
      nrecovered = rbinom(n=1,size=current[2],prob=gamma)
      current[1] = current[1] - ninfected
      current[2] = current[2] + ninfected - nrecovered
      current[3] = current[3] + nrecovered
      if(delta_D > 0){
        ninfected = rbinom(n=1,size=current[5],prob=effective_beta)
        nrecovered = rbinom(n=1,size=current[6],prob=(1/(1/gamma - delta_D)))
        current[5] = current[5] - ninfected
        current[6] = current[6] + ninfected - nrecovered
        current[3] = current[3] + nrecovered
      }
    }
    Rprof(NULL)
    tmp = summaryRprof()
    times[trial] = tmp$sampling.time
    # This needs to happen twice, once with and once without the intervention so multiply all times by 2.
  }
  return(all_results)
}

library(abind)
run_all_mw_inference <- function(ntime,npop,ninf,ntrial){
  rc <- abind(
    run_mw_inference(ntime,npop,ninf,ntrial,R0*sigma),
    run_mw_inference(ntime,npop,ninf,ntrial,R0,vcr = vaccination_rate),
    run_mw_inference(ntime,npop,ninf,ntrial,R0,delta_D = sick_reduction),
    run_mw_inference(ntime,npop,ninf,ntrial,R0),
    run_mw_inference(ntime,npop,ninf,ntrial,R0),
    along=4)
  dimnames(rc)[[4]] = c("Hand Washing","Vaccination","Antivirals","No Intervention","Uncontrolled")
  return(rc)
}
library(reshape2)
library(tidyr)
mw_output = melt(run_all_mw_inference(ntime,sum(init),init[2],ntrial)) %>% as_data_frame()
names(mw_output) = c("trial",'t','variable','scenario','value')
mw_output$type = 'Traditional'
mw_output = spread(mw_output,variable,value)
mw_output$S = mw_output$S + mw_output$S_a
mw_output$I = mw_output$I + mw_output$I_a
mw_output$S_a = NULL
mw_output$I_a = NULL
mw_output = gather(mw_output,variable,value,S,I,R,V)
mw_residuals = spread(mw_output,scenario,value)
mw_residuals$`Hand Washing` = mw_residuals$`Hand Washing` - mw_residuals$Uncontrolled
mw_residuals$`Antivirals` = mw_residuals$`Antivirals` - mw_residuals$Uncontrolled
mw_residuals$`Vaccination` = mw_residuals$`Vaccination` - mw_residuals$Uncontrolled
mw_residuals$`No Intervention` = mw_residuals$`No Intervention` - mw_residuals$Uncontrolled
mw_residuals$Uncontrolled =NULL
mw_residuals = gather(mw_residuals,scenario,value,`Hand Washing`,Antivirals,`No Intervention`,`Vaccination`)

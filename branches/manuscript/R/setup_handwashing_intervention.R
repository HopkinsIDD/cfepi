source("R/intervention_params.R")
## Social Distancing
sigma = .99 # Use mask values
beta_pars <- list(start_time = tstar, rate= 1-sigma)
susceptible_pars <- list()
run_scenario(
  output_name1,
  init,
  "Flat",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

source("R/intervention_params.R")
run_scenario(
  output_name2,
  init,
  "None",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)
## none
beta_pars <- list()
susceptible_pars <- list()
run_scenario(
  output_name1,
  init,
  "None",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

options(error=recover)
options(warn=1)
source("R/intervention_params.R")
## antivirals
beta_pars <- list()
move_to = 2
move_from = 1
susceptible_pars <- list(intervention_time = tstar, coverage = prop, rate = antiviral_rate, to=move_to, from=move_from)
run_scenario(
  output_name1,
  init,
  "None",
  "Constant",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

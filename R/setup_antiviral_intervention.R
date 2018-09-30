options(error=recover)
options(warn=1)
source("R/intervention_params.R")
## antivirals
sick_reduction = 16.8/24 #When it works reduces infection by 16.8 hours
rate = (1/(mu - sick_reduction) - gamma)/(1-gamma)
beta_pars <- list()
move_to = 2
move_from = 1
susceptible_pars <- list(intervention_time = tstar, coverage = prop, rate = rate, to=move_to, from=move_from)
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

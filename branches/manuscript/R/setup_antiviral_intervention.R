source("R/intervention_params.R")
## antivirals
sick_reduction = 16.8/24 #When it works reduces days infected by 1.5
rate = (1/(mu - sick_reduction) - gamma)/(1-gamma) * prop
beta_pars <- list()
move_to = 2
move_from = 1
susceptible_pars <- list(intervention_time = tstar, rate = rate, to=move_to, from=move_from)
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

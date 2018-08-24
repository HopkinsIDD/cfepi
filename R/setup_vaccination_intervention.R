source("R/intervention_params.R")
## Vaccination
effective_rate = .33 #Works 33% of the time
rate = (effective_rate) * prop
move_to = 3
move_from = 0
susceptible_pars <- list(intervention_time = tstar, rate = prop, to=move_to,from=move_from)
run_scenario(
  output_name1,
  init,
  "None",
  "Single",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

source("R/intervention_params.R")
## Vaccination
move_to = 3
move_from = 0
susceptible_pars <- vaccination_pars
run_scenario(
  output_name1,
  paste("Vaccination",paste(gsub('.','',unlist(susceptible_pars),fixed=T),collapse='-'),sep='-'),
  init,
  "None",
  "Single",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

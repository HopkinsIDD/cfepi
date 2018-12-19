source("R/intervention_params.R")
## antivirals
beta_pars <- list()
move_to = 2
move_from = 1
susceptible_pars <- antiviral_pars

run_scenario(
  output_name1,
  paste("Antivirals",paste(gsub('.','',unlist(susceptible_pars),fixed=T),collapse='-'),sep='-'),
  init,
  "None",
  "Constant",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

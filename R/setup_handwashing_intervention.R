source("R/intervention_params.R")
## Social Distancing
beta_pars <- hand_washing_pars
susceptible_pars <- list()
run_scenario(
  output_name1,
  paste("Hand Washing",paste(gsub('.','',unlist(beta_pars),fixed=T),collapse='-'),sep='-'),
  init,
  "Flat",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

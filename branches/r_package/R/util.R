#' @export
#' @name run_full_counterfactual
#' @title run_full_counterfactual
run_full_counterfactual <- function(
  filename,
  init,
  inter,
  trans,
  beta_type,
  susceptible_type,
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial = 1000){
  setup_counterfactual(filename,init,inter,trans,ntime,ntrial)
  run_scenario(filename,init,beta_type,susceptible_type,beta_pars,susceptible_pars,ntime,ntrial = 1000)
  run_scenario(filename,init,"None","None",list(),list(),ntime,ntrial = 1000)
}


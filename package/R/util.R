#' @export
#' @name run_full_counterfactual
#' @title run_full_counterfactual
#' @description Setup and run a full counterfactual scenario.  Calls \code{setup_counterfactual} and \code{run_counterfactual}.
#' @param filename A string containing the desired output stub.
#'   Should contain a filepath (probably to a directory) where the
#'   output from the counterfactual should be stored.
#' @param intervention_name A string containing a unique name for the
#'   intervention.  Any interventions with the same name will use the 
#'   same files, so take care not to overwrite an intervention by
#'   mistake
#' @param init A numeric vector of the initial conditions for the
#'   model.  It should length equal to the number of variables in the
#'   differential equations model, and each value should be the
#'   initial number of people in that compartment.
#' @param trans A matrix containing the state transitions that happen
#'   independent of any human interactions.  This matrix should be
#'   n by n, where n is the number of compartments. \code{trans[i,j]}
#'   should be the probability of transitioning from state \code{i} to
#'   state \code{j} at every time step.
#' @param inter An array containing the state transitions that happen
#'   as a result of human interactions.  This array should be n by n
#'   by n, where n is the number of compartments. \code{trans[i,j,k]}
#'   should be the probability of someone in state i having an
#'   interaction with a particular person in state \code{j}, which
#'   causes them to transition to state \code{k} at each time step.
#' @param beta_type One of "None" or "Flat".
#'   "None" means that the intervention should not reduce
#'   transmissibility.
#'   "Flat" means that the intervention should reduce transmissibility
#'   by a constant \code{rate} after a certain \code{start_time}.
#' @param susceptible_type One of "None" or "Single".
#'   "None" means that the intervention should not eliminate
#'   susceptibles
#'   "Single" means that the intervention should eliminate susceptibles
#'   at a single \code{start_time} moving \code{rate} of them from 
#'   state \code{from_state} to state \code{to_state}.
#' @param beta_pars The parameters to use for the transmission
#'   reduction function.  This should be a list.
#' @param susceptible_pars the parameters to use for the susceptible 
#'   elimination function.  This should be a list.
#' @param ntime  The number of timesteps to run the simulation over.
#' @param ntrial The number of independent simulations to run.  The 
#'   differential equation is converted to a stochastic process when
#'   making it agent based, and so multiple trials are recommended.
#' @return This function generates \code{2*ntrial} files where
#'   determined by \code{filename}
run_full_counterfactual <- function(
  filename,
  intervention_name,
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
  run_scenario(filename,intervention_name,init,beta_type,susceptible_type,beta_pars,susceptible_pars,ntime,ntrial)
  run_scenario(filename,"No Intervention",init,"None","None",list(),list(),ntime,ntrial)
  return(read_scenario(filename))
}

#' @export
#' @name calculate_residual
#' @title calculate_residual
#' @description Compare each intervention in a scenario to the Uncontrolled version.
#' @param counterfactual_frame  The data frame returned by read_scenario
#' @param base_name Name of a column in the counterfactual_frame to compare other columns to.  Defaults to None_None (or no intervention).
calculate_residual = function(counterfactual_frame,base_name='No Intervention'){
  all_interventions = as.character(unique(counterfactual_frame$intervention))
  #' @importFrom tidyr spread_
  counterfactual_frame = spread_(counterfactual_frame,'intervention','value')
  if(!(base_name %in% names(counterfactual_frame))){
    stop("In order to calculate residuals, an uncontrolled epidemic is required")
  }
  if('Uncontrolled' %in% names(counterfactual_frame)){
    warning("Uncontrolled should not be an intervention name.")
  }
  counterfactual_frame$Uncontrolled = counterfactual_frame[[base_name]]
  for(intervention in all_interventions){
    counterfactual_frame[[intervention]] = counterfactual_frame[[intervention]] - counterfactual_frame$Uncontrolled
  }
  #' @importFrom tidyr gather_
  counterfactual_frame = gather_(counterfactual_frame,key_col='intervention',value_col='value',gather_cols=all_interventions)
  #' @importFrom dplyr select_
  counterfactual_frame = select_(counterfactual_frame,'-Uncontrolled')
  return(counterfactual_frame)
}

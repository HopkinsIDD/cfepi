#' @export 
#' @name run_scenario
#' @title run_scenario
#' @description Using a previously setup counterfactual, run a
#'   particular scenario.
#' @param filename A string containing the desired output stub.
#'   Should be the same argument that was passed to
#'   \code{setup_counterfactual}.
#' @param init A numeric vector of the initial conditions for the
#'   model.  It should length equal to the number of variables in the
#'   differential equations model, and each value should be the
#'   initial number of people in that compartment.  Should be the same
#'   argument that was passed to \code{setup_counterfactual}.
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
#'   Should be the same argument that was passed to
#'   \code{setup_counterfactual}
#' @param ntrial The number of independent simulations to run.  The 
#'   differential equation is converted to a stochastic process when
#'   making it agent based, and so multiple trials are recommended.
#'   Should be the same argument that was passed to 
#'   \code{setup_counterfactual}
#' @return This function returns a data.frame one column for each
#'   state and one column for the simulation number.  It has one row
#'   for each time step, and the values are the number of people in
#'   that state at that time.
run_scenario <- function(filename,init,beta_type,susceptible_type,ntime,ntrial = 1000){
  .Call('runIntervention',
    filename,
    init,
    beta_type,
    susceptible_type,
    beta_pars,
    susceptible_pars,
    ntime, 
    ntrial,
    PACKAGE='counterfactual'
  )
}

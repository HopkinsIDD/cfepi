#' @export 
#' @name setup_counterfactual
#' @title setup_counterfactual
#' @description Run all possibly relevant transmissions for a
#'   particular differential equations model.
#' @param filename A string containing the desired output stub.
#'   Should contain a filepath (probably to a directory) where the
#'   output from the counterfactual should be stored.
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
#' @param ntime  The number of timesteps to run the simulation over.
#' @param ntrial The number of independent simulations to run.  The 
#'   differential equation is converted to a stochastic process when
#'   making it agent based, and so multiple trials are recommended.
#' @return This function generates \code{2*ntrial} files where
#'   determined by \code{filename}
setup_counterfactual <- function(filename,init,inter,trans,ntime,ntrial = 1000){
  .Call('setupCounterfactualAnalysis',
    filename,
    init,
    inter,
    trans,
    ntime,
    ntrial,
    PACKAGE='counterfactual'
  )
}

% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/util.R
\name{run_full_counterfactual}
\alias{run_full_counterfactual}
\title{run_full_counterfactual}
\usage{
run_full_counterfactual(filename, intervention_name, init, inter, trans,
  beta_type, susceptible_type, beta_pars, susceptible_pars, ntime,
  ntrial = 1000)
}
\arguments{
\item{filename}{A string containing the desired output stub.
Should contain a filepath (probably to a directory) where the
output from the counterfactual should be stored.}

\item{intervention_name}{A string containing a unique name for the
intervention.  Any interventions with the same name will use the 
same files, so take care not to overwrite an intervention by
mistake}

\item{init}{A numeric vector of the initial conditions for the
model.  It should length equal to the number of variables in the
differential equations model, and each value should be the
initial number of people in that compartment.}

\item{inter}{An array containing the state transitions that happen
as a result of human interactions.  This array should be n by n
by n, where n is the number of compartments. \code{trans[i,j,k]}
should be the probability of someone in state i having an
interaction with a particular person in state \code{j}, which
causes them to transition to state \code{k} at each time step.}

\item{trans}{A matrix containing the state transitions that happen
independent of any human interactions.  This matrix should be
n by n, where n is the number of compartments. \code{trans[i,j]}
should be the probability of transitioning from state \code{i} to
state \code{j} at every time step.}

\item{beta_type}{One of "None" or "Flat".
"None" means that the intervention should not reduce
transmissibility.
"Flat" means that the intervention should reduce transmissibility
by a constant \code{rate} after a certain \code{start_time}.}

\item{susceptible_type}{One of "None" or "Single".
"None" means that the intervention should not eliminate
susceptibles
"Single" means that the intervention should eliminate susceptibles
at a single \code{start_time} moving \code{rate} of them from 
state \code{from_state} to state \code{to_state}.}

\item{beta_pars}{The parameters to use for the transmission
reduction function.  This should be a list.}

\item{susceptible_pars}{the parameters to use for the susceptible 
elimination function.  This should be a list.}

\item{ntime}{The number of timesteps to run the simulation over.}

\item{ntrial}{The number of independent simulations to run.  The 
differential equation is converted to a stochastic process when
making it agent based, and so multiple trials are recommended.}
}
\value{
This function generates \code{2*ntrial} files where
  determined by \code{filename}
}
\description{
Setup and run a full counterfactual scenario.  Calls \code{setup_counterfactual} and \code{run_counterfactual}.
}

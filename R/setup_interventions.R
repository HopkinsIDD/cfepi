try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
library(counterfactual)


## Set up an SIR model with given parameters
# beta = .2
# gamma = .1
# npop = 4000000
# ntime = 365
# ntrial = 1000
R0 = 1.75 #
mu = 2.25 #days
gamma = 1/mu
beta = R0 * gamma
npop = 400000 #For testing only
ntime = 365
warning("Using testing values for number of trials")
ntrial = 10 #1000 for final figures

output_name1 = 'output/figures0'
output_name2 = 'output/figures1'

warning("Using testing directories")
output_name1 = 'old_output/figures0'
output_name2 = 'old_output/figures1'

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - 10,10,0)
inter <- inter/npop
# This code is commented out as it takes a long time to run, and need only run a single time

## cross intervention parameters
prop = .25 # 25% adoption rate for intervention
tstar = 1 #start on day 1

## none
beta_pars <- list()
susceptible_pars <- list()
run_scenario(
  output_name1,
  init,
  "None",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)
run_scenario(
  output_name2,
  init,
  "None",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

## antivirals
sick_reduction = 16.8/24 #When it works reduces days infected by 1.5
rate = (1 - 1/sick_reduction) * prop
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
## Social Distancing
sigma = .99 # Use mask values
tstar = 0
beta_pars <- list(start_time = tstar, rate= 1-sigma)
susceptible_pars <- list()
run_scenario(
  output_name1,
  init,
  "Flat",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

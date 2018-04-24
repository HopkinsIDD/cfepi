try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
library(counterfactual)


## Set up an SIR model with given parameters
# beta = .2
# gamma = .1
# npop = 4000000
# ntime = 365
# ntrial = 1000
beta = .2
gamma = .1
npop = 4000
ntime = 30
ntrial = 10

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - 10,10,0)
inter <- inter/npop
setup_counterfactual(
  'figures/output/figures',
  init,
  inter,
  trans,
  ntime,
  ntrial
)

## none
beta_pars <- list(start_time = 30,rate= .05)
susceptible_pars <- list()
run_scenario(
  'figures/output/figures',
  init,
  "None",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

## Treatment
prop = .1
tstar = 30
beta_pars <- list()
susceptible_pars <- list(rate = prop, intervention_time = tstar, from=2,to=3)
run_scenario(
  'figures/output/figures',
  init,
  "None",
  "Single",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)
## Vaccination
prop = .3
tstar = 30
susceptible_pars <- list(rate = prop, intervention_time = tstar, from=1,to=3)
susceptible_pars <- list()
run_scenario(
  'figures/output/figures',
  init,
  "None",
  "Single",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)
## Social Distancing
sigma = .95
tstar = 30
beta_pars <- list(start_time = 30,rate= 1-sigma),
susceptible_pars <- list()
run_scenario(
  'figures/output/figures',
  init,
  "Flat",
  "None",
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial
)

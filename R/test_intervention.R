try({remove.packages('cfepi')},silent=T)
install.packages('package',type='source',repos=NULL)
library(cfepi)

source("R/intervention_params.R")

npop = 4000
ntime = 200
ntrial = 50

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - 10,10,0)
inter <- inter/npop
# This code is commented out as it takes a long time to run, and need only run a single time
output_name1 = 'output/test0'
output_name2 = 'output/test1'
# setup_counterfactual(
#   output_name1,
#   init,
#   inter,
#   trans,
#   ntime,
#   ntrial
# )
# setup_counterfactual(
#   output_name2,
#   init,
#   inter,
#   trans,
#   ntime,
#   ntrial
# )
# 
# 
# run_scenario(
#   output_name2,
#   init,
#   "None",
#   "None",
#   beta_pars,
#   susceptible_pars,
#   ntime,
#   ntrial
# )
# ## none
# beta_pars <- list()
# susceptible_pars <- list()
# run_scenario(
#   output_name1,
#   init,
#   "None",
#   "None",
#   beta_pars,
#   susceptible_pars,
#   ntime,
#   ntrial
# )
## Vaccination
effective_rate = .33 #Works 33% of the time
rate = (effective_rate) * prop
move_to = 3
move_from = 0
susceptible_pars <- list(intervention_time = tstar, rate = rate, to=move_to,from=move_from)
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

sick_reduction = 16.8/24 #When it works reduces days infected by 1.5
rate = (1/(mu - sick_reduction) - gamma)/(1-gamma) * prop
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

## Social Distancing
sigma = .99 # Use mask values
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


output = read_scenario(output_name1,ntrial)
multiworld_output = read_scenario(output_name2,ntrial)

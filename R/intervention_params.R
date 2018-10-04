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
D = 2.25 #days
gamma = 1/D
beta = R0 * gamma
npop = 4000 #For testing only
ntime = 100
# warning("Using testing values for number of trials")
ntrial = 1000 #1000 for final figures
ninf=5

output_name1 = 'output/figures0'
output_name2 = 'output/figures1'

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - ninf,ninf,0)
inter <- inter/npop
# This code is commented out as it takes a long time to run, and need only run a single time

## cross intervention parameters
prop = .10 # 25% adoption rate for intervention
tstar = 1 #start on day 1

## Default intervention parameters
beta_pars <- list()
susceptible_pars <- list()

## Specific intervention params
sick_reduction = 16.8/24 #When it works reduces infection by 16.8 hours
antiviral_rate = 1/(D- sick_reduction) - gamma
sigma = .95 # Use mask values
effective_rate = .33 #Works 33% of the time
vaccination_rate = (effective_rate) * prop


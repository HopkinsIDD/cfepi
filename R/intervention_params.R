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
npop = 4000 #For testing only
ntime = 100
# warning("Using testing values for number of trials")
ntrial = 1000 #1000 for final figures

output_name1 = 'output/figures0'
output_name2 = 'output/figures1'

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - 5,5,0)
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
antiviral_rate = (1/(mu - sick_reduction) - gamma)/(1-gamma)
sigma = .94 # Use mask values
effective_rate = .33 #Works 33% of the time
vaccination_rate = (effective_rate) * prop

testing = TRUE
final_size_expected = c()
if(testing){
  scenarios = c("Null","Antivirals","Handwashing","Vaccination")
  modified_R0 = c()
  modified_R0['Antivirals'] = prop*(R0/(1+antiviral_rate)) + (1-prop)*R0
  modified_R0['Handwashing'] = R0*sigma
  modified_R0['Vaccination'] = R0*(1-vaccination_rate)
  modified_R0['Null'] = R0
  for(scenario in scenarios){
    tmp_R0 = modified_R0[scenario]
    tester = function(r.inf) {abs(r.inf - (1-exp(-tmp_R0*r.inf)))}
    final_size_expected[scenario] = optim(fn=tester,par=c(r.inf=1))$par
  }
  final_size_expected = final_size_expected * c(npop,npop,npop,npop*(1-vaccination_rate))
}

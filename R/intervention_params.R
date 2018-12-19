if(!('reinstalled' %in% ls())){
  options(warn=1)
  try({remove.packages('cfepi')},silent=T)
  install.packages('package',type='source',repos=NULL)
  library(cfepi)
  reinstalled =TRUE
}
source('R/counterfactual_params.R')

## cross intervention parameters
prop = as.numeric(args[6]) # 10% adoption rate for intervention
tstar = as.numeric(args[7]) #start on day 1

## Default intervention parameters
beta_pars = list()
susceptible_pars = list()

## Specific intervention params
sick_reduction = 16.8/24 #When it works reduces infection by 16.8 hours
gamma_av = 1/(D-sick_reduction)
antiviral_rate = (gamma_av - gamma)/(1-gamma)
print(paste("AV=",antiviral_rate))
sigma = .95 # Use mask values
effective_rate = .33 #Works 33% of the time
vaccination_rate = (effective_rate) * prop

## Putting them into the right format
hand_washing_pars = list(start_time = tstar, rate= 1-sigma)
vaccination_pars = list(intervention_time = tstar, rate = vaccination_rate, to=3,from=0)
antiviral_pars = list(intervention_time = tstar, coverage = prop, rate = antiviral_rate, to=2, from=1)

## Name the interventions
hw_name = paste("Hand Washing",paste(gsub('.','',unlist(hand_washing_pars),fixed=T),collapse='-'),sep='-')
va_name = paste("Vaccination",paste(gsub('.','',unlist(vaccination_pars),fixed=T),collapse='-'),sep='-')
av_name = paste("Antivirals",paste(gsub('.','',unlist(antiviral_pars),fixed=T),collapse='-'),sep='-')
ni_name = "No Intervention"
intervention = c(hw_name,va_name,av_name,ni_name)

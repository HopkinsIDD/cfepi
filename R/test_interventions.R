source("R/intervention_params.R")
library(cfepi)
library(dplyr)

testing = TRUE
final_size_expected = c()
sick_reduction = .7 
if(testing){
  scenarios = c("Null","Antivirals","Handwashing","Vaccination")
  modified_R0 = c()
  modified_R0['Antivirals'] = prop*(beta/(1/(1/gamma - sick_reduction))) + (1-prop)*R0
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
output = read_scenario("output/figures0")
final_size_actual = output %>%
  filter(t == 100,V3 > 100) %>%
  group_by(beta_name,susceptible_name) %>%
  summarize_all(funs(mean))

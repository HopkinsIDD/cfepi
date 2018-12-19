source("R/intervention_params.R")
library(cfepi)
library(dplyr)

## Rename scenarios for printing
scenario_changer = as.factor(c(
  'None_None' = 'No Intervention',
  'None_Constant' = 'Antivirals',
  'Flat_None' = 'Hand Washing',
  'None_Single' = 'Vaccination'
))
variable_changer = as.factor(c(
  'V1' = 'S',
  'V2' = 'I',
  'V3' = 'R',
  'V4' = 'V'
))

testing = TRUE
final_size_expected = c()
sick_reduction = .7 
if(testing){
  scenarios = scenario_changer
  modified_R0 = c()
  modified_R0['Antivirals'] = prop*(beta/gamma_av) + (1-prop)*R0
  modified_R0['Hand Washing'] = R0*sigma
  modified_R0['Vaccination'] = R0*(1-vaccination_rate)
  modified_R0['No Intervention'] = R0
  for(scenario in scenarios){
    tmp_R0 = modified_R0[scenario]
    tester = function(r.inf) {abs(r.inf - (1-exp(-tmp_R0*r.inf)))}
    final_size_expected[scenario] = optim(fn=tester,par=c(r.inf=1),method='Brent',lower=0,upper=1)$par
  }
  final_size_expected = final_size_expected * c(npop,npop,npop,npop*(1-vaccination_rate))
}
# output = read_scenario("output/figures0")
# tmp = output %>%
#   mutate(scenario = scenario_changer[scenario],variable = variable_changer[variable]) %>%
#   filter(t == 100,variable == 'R') %>% # Final number of Recovered
#   group_by(scenario) %>%
#   summarize(value = mean(value))
# final_size_actual = setNames(tmp$value,tmp$scenario)

# print(final_size_actual)
print(final_size_expected)

# > names(output)
# [1] "scenario" "trial"    "t"        "variable" "value"

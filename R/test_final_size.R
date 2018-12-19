options(warn=1)
options(error=function(){quit(1)})
# Define relevent directories
working_directory = rprojroot::find_root(rprojroot::has_file('.root')) # The location of the repository
setwd(working_directory)

try({remove.packages('cfepi')},silent=T)
install.packages('package',type='source',repos=NULL)
# try({remove.packages('counterfactual')},silent=T)
# install.packages('package',type='source',repos=NULL)

## Some parameters:
ci_width = .9
alpha = 1-ci_width


if(!require(cfepi)){
  source("package/R/read.R")
  source("package/R/utils.R")
}
# library(cfepi)
library(cowplot)
library(ggplot2)
library(dplyr)
library(tidyr)
library(grid)
library(foreach)


#This provides mw_output and mw_residuals
all_R0 = c(1.25,1.75,2.25)
all_ii = c(1,5,10)
all_prop = c(.05,.1,.3)
all_start_time = c(1,10,30)

warning("Testing only with some interventions")
all_R0 = all_R0[2]
all_ii = all_ii[2]
all_prop = all_prop[2]
all_start_time = all_start_time[1]

all_scenarios = c()
all_interventions = c()

for(ro in all_R0){
  for(ii in all_ii){
    for(prop in all_prop){
      for(t in all_start_time){
        args = c(4000,100,1000,ro,ii,prop,t)
        source('R/intervention_params.R')
        if(any(grepl("NA",all_scenarios))){stop("This should not happen")}
        all_scenarios = unique(c(all_scenarios,scenario))
        all_interventions = unique(c(all_interventions,intervention))
      }
    }
  }
}

all_interventions = factor(all_interventions)
tmp = which(levels(all_interventions) == 'No Intervention')
new_order = c(tmp,(1:length(levels(all_interventions)))[-tmp])
all_interventions = sort(factor(all_interventions,levels=levels(all_interventions)[new_order]))



all_scenarios = all_scenarios[grepl('main',all_scenarios)]

args = c(4000,100,1000,1.75,5,.1,1)
ntrials = args[3]
source("R/intervention_params.R")
scenario = scenario[grepl('main',scenario)]
tmp = read_scenario(scenario,intervention,ntrials,'output')
output = tmp
output$intervention = factor(output$intervention,levels=all_interventions)
output = filter(output,!is.na(value))

type_changer = as.factor(c(
  'Multi_World' = 'Traditional',
  'Single_World' = 'Single-World'
))
type_changer = relevel(type_changer,2,1)

variable_changer = as.factor(c(
  'V1' = 'S',
  'V2' = 'I',
  'V3' = 'R',
  'V4' = 'V'
))
variable_changer = relevel(variable_changer,3,1,2,4)

output$variable = variable_changer[output$variable]
output$type = 'Single-World'
output$trial = as.numeric(output$trial)

tmprow = output[1,]
tmprow$value = NA
tmprow$variable='V'
output= rbind(output,tmprow)
named_diff = function(x,y){
  return(setNames(x-y,paste(names(x),names(y))))
}
epivar_estimates = function(input,max.trials = 1000){
  input %>%
    filter(trial<=max.trials) %>%
    spread(variable,value) %>%
    group_by(scenario,intervention,trial) %>%
    do({
      tmp = arrange(.,t)
      tmp$delS = c(diff(tmp$S),0)
      tmp$delI = c(diff(tmp$I),0)
      tmp$delR = c(diff(tmp$R),0)
      tmp$delV = c(diff(tmp$V),0)
      tmp$beta = exp(log(-tmp$delS) - (log(tmp$S) + log(tmp$I)))
      tmp$gamma = exp(log(tmp$delR) - log(tmp$I))
      tmp$final_size = tmp$R[nrow(tmp)]
      tmp$final_infected = tmp$I[nrow(tmp)]
      tmp
    }) %>%
    filter(final_size > 200,final_infected==0) %>%
    summarize(
      gamma = median(gamma,na.rm=T),
      beta = median(beta,na.rm=T),
      final_size = max(R,na.rm=T)
    ) %>%
    ungroup() %>%
    group_by(scenario,intervention) %>%
    summarize(
      gamma = mean(gamma),
      beta = mean(beta),
      beta2 = beta * 4000,
      final_size = mean(final_size)
    ) %>%
    arrange(scenario,intervention) %>% 
    return
}

lhs = epivar_estimates(output,ntrials)
# rhs = epivar_estimates(all_mw_output,ntrials)
source("R/test_interventions.R")

# sorted_final_size = final_size_expected[sort(names(final_size_expected))]
sorted_final_size = final_size_expected

print(named_diff(sorted_final_size,setNames(lhs$final_size,lhs$intervention)))
# print(named_diff(sorted_final_size,setNames(rhs$final_size,rhs$intervention)))

print(named_diff(7/9,setNames(lhs$beta2,lhs$intervention)))
# print(named_diff(7/9,setNames(rhs$beta2,rhs$intervention)))

print(named_diff(4/9,setNames(lhs$gamma,lhs$intervention)))
# print(named_diff(4/9,setNames(rhs$gamma,rhs$intervention)))
exit()


make_mean_timeseries_dieout = function(input,max.trials){
  input %>%
    filter(trial <= max.trials) %>%
    spread(variable,value) %>%
    group_by(scenario,intervention,trial) %>%
    do({
      tmp = arrange(.,t)
      tmp$delS = c(diff(tmp$S),0)
      tmp$delI = c(diff(tmp$I),0)
      tmp$delR = c(diff(tmp$R),0)
      tmp$delV = c(diff(tmp$V),0)
      tmp$beta = exp(log(-tmp$delS) - (log(tmp$S) + log(tmp$I)))
      tmp$gamma = exp(log(tmp$delR) - log(tmp$I))
      tmp$final_size = max(tmp$R)
      tmp$final_infected = tmp$I[nrow(tmp)]
      tmp
    }) %>%
    filter(final_size <= 100,final_infected==0) %>%
    ungroup() %>%
    gather(variable,value,delS,delI,delR,delV,S,I,R,V,beta,gamma,final_size,final_infected) %>%
    group_by(scenario,intervention,t,variable) %>%
    summarize(value = mean(value,na.rm=T),value_l = quantile(value,.025,na.rm=T),value_h = quantile(value,.975,na.rm=T)) %>%
    return
}

lhs2 = make_mean_timeseries_dieout(output,ntrials)
rhs2 = make_mean_timeseries_dieout(all_mw_output,ntrials)

plt_dieout = ggplot() +
  geom_line(data = filter(lhs2, variable == 'I'), aes(x=t,y=value,col=intervention,linetype='Single-world')) + 
  geom_line(data = filter(rhs2, variable == 'I'), aes(x=t,y=value,col=intervention,linetype='Traditional'))

make_mean_timeseries_no_dieout = function(input,max.trials){
  input %>%
    filter(trial <= max.trials) %>%
    spread(variable,value) %>%
    group_by(scenario,intervention,trial) %>%
    do({
      tmp = arrange(.,t)
      tmp$delS = c(diff(tmp$S),0)
      tmp$delI = c(diff(tmp$I),0)
      tmp$delR = c(diff(tmp$R),0)
      tmp$delV = c(diff(tmp$V),0)
      tmp$beta = exp(log(-tmp$delS) - (log(tmp$S) + log(tmp$I)))
      tmp$gamma = exp(log(tmp$delR) - log(tmp$I))
      tmp$final_size = max(tmp$R)
      tmp$final_infected = tmp$I[nrow(tmp)]
      tmp
    }) %>%
    filter(final_size > 100,final_infected==0) %>%
    ungroup() %>%
    gather(variable,value,delS,delI,delR,delV,S,I,R,V,beta,gamma,final_size,final_infected) %>%
    group_by(scenario,intervention,t,variable) %>%
    summarize(value = mean(value,na.rm=T),value_l = quantile(value,.025,na.rm=T),value_h = quantile(value,.975,na.rm=T)) %>%
    return
}

lhs3 = make_mean_timeseries_no_dieout(output,ntrials)
rhs3 = make_mean_timeseries_no_dieout(all_mw_output,ntrials)

plt_no_dieout = ggplot() +
  geom_line(data = filter(lhs3, variable == 'I'), aes(x=t,y=value,col=intervention,linetype='Single-world')) + 
  geom_line(data = filter(rhs3, variable == 'I'), aes(x=t,y=value,col=intervention,linetype='Traditional')) + 
  scale_y_log10()

pdf('~/tmp.pdf')
print(plt_dieout)
print(plt_no_dieout)
dev.off()



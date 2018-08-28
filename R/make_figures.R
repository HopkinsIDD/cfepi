try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
# try({remove.packages('counterfactual')},silent=T)
# install.packages('package',type='source',repos=NULL)
library(counterfactual)
library(cowplot)
library(ggplot2)
library(dplyr)
library(tidyr)
library(grid)

output = read_scenario('output/figures0')
multiworld_output = read_scenario('output/figures1')

scenario_changer = c(
  'Flat_None' = 'Social Distancing',
  'None_None' = 'Null',
  'None_Single' = 'Vaccination',
  'None_Constant' = 'Antivirals'
)

## Figure 1 - Illustration of the Problem
### Simulated Epidemic Curves with and without intervention
    # color intervention
plt = output %>%
  unite(scenario,beta_name,susceptible_name) %>%
  mutate(V4 = 400000 - V1 - V2 - V3) %>%
  gather(Variable,Value,V1,V2,V3,V4) %>% 
  rename(People = Value,Time = t) %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  group_by(scenario,Time,Variable) %>%
  summarize(People_L = quantile(People,.025), People_H = quantile(People,.975), People = mean(People)) %>% 
  filter(Variable == "V2") %>%
  mutate(Variable = "Number Infected") %>%
  ggplot() +
  geom_line(aes(
    x=Time,
    y=People,
    color=Variable,
    group=Variable
  )) +
  geom_ribbon(aes(
    x=Time,
    ymin=People_L,
    ymax=People_H,
    fill=Variable,
    group=Variable
  ),alpha=.7) +
  xlab("Time (Days)") +
  facet_wrap(~scenario)
pdf('figures/epicurve.pdf')
print(plt)
dev.off()

scenario_changer = c(
  'Flat_None' = "Social Distancing",
  'None_None' = "Null",
  'None_Single' = "Vaccination",
  'None_Constant' = "Antivirals"
)

#### Final Size
final_size = function(x){
x %>%
  filter(t == max(t))  %>%
  unite(scenario,beta_name,susceptible_name) %>%
  select(scenario,trial,final_size=V3) %>%
  return()
}

time_series = function(x){
x %>%
  unite(scenario,beta_name,susceptible_name) %>%
  mutate(V4 = 400000-V1-V2-V3) %>% 
  gather(variable,value,V1,V2,V3,V4) %>%
  select(scenario,trial,final_size=value,t,variable) %>%
  return()
}

peak_time = function(x){
x %>%
  unite(scenario,beta_name,susceptible_name) %>%
  group_by(scenario,trial) %>%
  do({
    tmp = .
    tmp$max_V2 = max(.$V2)
    tmp
  }) %>%
  ungroup() %>%
  filter(V2 == max_V2)  %>%
  select(scenario,trial,peak_time = t) %>%
  group_by(scenario,trial) %>%
  summarize(peak_time = median(peak_time)) %>%
  return()
}

all_world_inference <- function(fun,name){
  rr = FALSE
  if(name == 'Log_Relative_Risk'){
    rr = TRUE
  }
  lhs = fun(output)
  names(lhs)[3] = name
  lhs = spread_(lhs,"scenario",name)
  rhs = fun(multiworld_output)
  names(rhs)[3] = name
  
  rhs = rhs[c('trial',name)]
  all_inference = inner_join(lhs,rhs,by=c('trial')) %>%
    rename_(.dots = setNames(name,'multi_world')) %>%
    mutate(single_world = None_None) %>%
    gather(type,null,single_world,multi_world)
  var_names = names(all_inference)[-c(1,length(all_inference) - 0:1)]
  if(rr){
    all_inference = all_inference %>%
      mutate_(.dots = setNames(paste('log(',var_names,' / null)'),var_names)) %>%
      gather_('scenario',name,var_names)
  } else {
    all_inference = all_inference %>%
      mutate_(.dots = setNames(paste(var_names,' - null'),var_names)) %>%
      gather_('scenario',name,var_names)
  }
  return(all_inference)
}


plot_cross_world <- function(fun,name){
  all_inference = all_world_inference(fun,name)
  if(
  ((max(all_inference[[name]]) - quantile(all_inference[[name]],.95)) > quantile(all_inference[[name]],.95)) ||
    ((quantile(all_inference[[name]],.05) - min(all_inference[[name]])) < min(all_inference[[name]],.95))
  ){
    rcin <- all_inference %>%
      mutate(scenario = scenario_changer[scenario]) %>%
      ggplot() +
      geom_boxplot(aes(x = scenario,y=all_inference[[name]],color = type)) +
      scale_colour_brewer(type='qual',palette='Paired') +
      theme(legend.position="none", aspect.ratio=0.9) +
      xlab("Scenario") +
      ylim(quantile(all_inference[[name]],c(.05,.95))) +
      ylab(gsub('_',' ',name)) +
      background_grid(major = "y", minor = "none")
    rcout <- all_inference %>%
      mutate(scenario = scenario_changer[scenario]) %>%
      ggplot() +
      geom_boxplot(aes(x = scenario,y=all_inference[[name]],color = type)) +
      scale_colour_brewer(type='qual',palette='Paired') +
      theme(legend.position="none", aspect.ratio=0.9) +
      xlab("Scenario") +
      ylab(gsub('_',' ',name)) +
      background_grid(major = "y", minor = "none")

    print(rcout)
    # inset = viewport(height=.5,width=1)
    # print(rcin,vp=inset)
    rc <- recordPlot()
  } else {
    rc <- all_inference %>%
      mutate(scenario = scenario_changer[scenario]) %>%
      ggplot() +
      geom_boxplot(aes(x = scenario,y=all_inference[[name]],color = type)) +
      scale_colour_brewer(type='qual',palette='Paired') +
      theme(legend.position="none", aspect.ratio=0.9) +
      xlab("Scenario") +
      ylab(gsub('_',' ',name)) +
      background_grid(major = "y", minor = "none")
  }
}
  

confidence_intervals = function(fun,name){
  tmp = all_world_inference(fun,name)
  tmp %>%
    group_by(type,scenario) %>%
    summarize_(.dots = setNames(c(paste0('quantile(',name,',.025)'), paste0('quantile(',name,',.975)'), paste0('mean(',name,')')),paste(name,c('l','h','m'),sep='_'))) %>%
    mutate_(.dots = setNames(paste0(name,'_h - ', name,'_l'),'width')) %>%
    return()
  
}

plot_inference = plot_cross_world

#### Peak Estimation
pdf('figures/intervention-effects-final-size.pdf')
print(plot_inference(final_size,'Final_Size'))
dev.off()
pdf('figures/intervention-effects-peak-time.pdf')
print(plot_inference(peak_time,'Peak_Time'))
dev.off()
pdf('figures/intervention-effects-relative-risk.pdf')
print(plot_inference(final_size,'Log_Relative_Risk'))
dev.off()

## Figure 2 - Cartoon/Diagram illustrating method.
### SIR diagram
#### Done in Latex
### Tree diagram of how intervention works.
#### Done in Latex
## Figure 3 - True Counterfactual vs Fake Counterfactual
### Relative Risk
    # x all interventions
    # y relative risk
    # color true counterfactual vs our methods
### Table 1 - Computational Resource Thresholds + Time

plt5 = final_size(output) %>%
  group_by(scenario) %>%
  ggplot() +
  geom_boxplot(aes(x=scenario,y=final_size))
pdf('figures/intervention-effects-raw-boxplots.pdf')
print(plt5)
dev.off()

plt6 = all_world_inference(final_size,'Final_Size') %>%
  filter(type == 'single_world') %>%
  ggplot +
  geom_boxplot(aes(x=scenario,y=Final_Size)) +
  facet_wrap(type~.)

pdf('figures/intervention-effects-combined-boxplots.pdf')
print(plt6)
while(dev.off() != 1){}

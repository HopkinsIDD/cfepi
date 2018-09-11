try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
# try({remove.packages('counterfactual')},silent=T)
# install.packages('package',type='source',repos=NULL)

# Define relevent directories
working_directory = rprojroot::find_root(rprojroot::has_file('.root')) # The location of the repository
setwd(working_directory)

if(!require(counterfactual)){
  source("package/R/read.R")
}
# library(counterfactual)
# library(cowplot)
library(ggplot2)
library(dplyr)
library(tidyr)
library(grid)

output = read_scenario('output/figures0')
multiworld_output = read_scenario('output/figures1')

scenario_changer = as.factor(c(
  'None_None' = 'Null',
  'None_Constant' = 'Antivirals',
  'Flat_None' = 'Social Distancing',
  'None_Single' = 'Vaccination'
))
scenario_changer = relevel(scenario_changer,2,1,3,4)

## Figure 1 - Illustration of the Problem
### Simulated Epidemic Curves with and without intervention
    # color intervention

npop = 4000

plt = output %>%
  unite(scenario,beta_name,susceptible_name) %>%
  mutate(V4 = npop - V1 - V2 - V3) %>%
  gather(Variable,Value,V1,V2,V3,V4) %>% 
  rename(People = Value,Time = t) %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  group_by(scenario,Time,Variable) %>%
  summarize(People_L = quantile(People,.025), People_H = quantile(People,.975), People = mean(People)) %>% 
  ungroup() %>%
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
  theme_bw() + 
  facet_wrap(~scenario)

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
  mutate(V4 = npop -V1-V2-V3) %>% 
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
  ungroup() %>%
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
  rhs = dplyr::select(rhs,-scenario)
  
  join_names = intersect(names(lhs),names(rhs))
  all_inference = inner_join(lhs,rhs,by=join_names) %>%
    rename_(.dots = setNames(name,'multi_world')) %>%
    mutate(single_world = None_None) %>%
    gather(type,null,single_world,multi_world)
  # var_names = names(all_inference)[-c(1,length(all_inference) - 0:1)]
  var_names = names(all_inference)[grepl('_',names(all_inference))]
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
      theme_bw() + 
      background_grid(major = "y", minor = "none") + 
      ylab(gsub('_',' ',name))
    rcout <- all_inference %>%
      mutate(scenario = scenario_changer[scenario]) %>%
      ggplot() +
      geom_boxplot(aes(x = scenario,y=all_inference[[name]],color = type)) +
      scale_colour_brewer(type='qual',palette='Paired') +
      theme(legend.position="none", aspect.ratio=0.9) +
      xlab("Scenario") +
      background_grid(major = "y", minor = "none") + 
      theme_bw() + 
      ylab(gsub('_',' ',name))

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
      background_grid(major = "y", minor = "none") + 
      theme_bw() + 
      ylab(gsub('_',' ',name))
  }
}
  

confidence_intervals = function(fun,name){
  tmp = all_world_inference(fun,name)
  tmp %>%
    group_by(type,scenario) %>%
    summarize_(.dots = setNames(c(paste0('quantile(',name,',.025)'), paste0('quantile(',name,',.975)'), paste0('mean(',name,')')),paste(name,c('l','h','m'),sep='_'))) %>%
    mutate_(.dots = setNames(paste0(name,'_h - ', name,'_l'),'width')) %>%
    ungroup() %>%
    return()
  
}

plot_inference = plot_cross_world

#### Peak Estimation
time_series_inference =all_world_inference(time_series,'Change_in_Cases')
time_series_summary = time_series_inference %>%
  group_by(t,variable,scenario,type) %>%
  summarize(`Change in Cases` = mean(Change_in_Cases),lq = quantile(Change_in_Cases,.025),uq = quantile(Change_in_Cases,.975))

plt_sus = time_series_summary %>%
  filter(variable == 'V1') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(scenario~type) +
  theme_bw() + 
  theme(legend.position="none")

plt_sus_t = time_series_summary %>%
  filter(variable == 'V1') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(type~scenario) +
  theme_bw() + 
  theme(legend.position="none")

plt_inf = time_series_summary %>%
  filter(variable == 'V2') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(scenario~type) +
  theme_bw() + 
  theme(legend.position="none")

plt_inf_t = time_series_summary %>%
  filter(variable == 'V2') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(type~scenario) +
  theme_bw() + 
  theme(legend.position="none")

plt_rec = time_series_summary %>%
  filter(variable == 'V3') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  geom_abline(slope=0,intercept=0,linetype=2) +
  facet_grid(scenario~type) +
  theme_bw() + 
  theme(legend.position="none")

plt_rec_t = time_series_summary %>%
  filter(variable == 'V3') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(type~scenario) +
  theme_bw() + 
  theme(legend.position="none")
pdf('figures/intervention-effects-final-size.pdf')
print(plot_inference(final_size,'Final_Size'))
dev.off()
pdf('figures/intervention-effects-peak-time.pdf')
print(plot_inference(peak_time,'Peak_Time'))
dev.off()
pdf('figures/intervention-effects-relative-risk.pdf')
print(plot_inference(final_size,'Log_Relative_Risk'))
dev.off()

plt_rec = time_series_summary %>%
  filter(variable == 'V3') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(scenario~type) +
  theme_bw() + 
  theme(legend.position="none")

plt_rec_t = time_series_summary %>%
  filter(variable == 'V3') %>%
  ungroup() %>%
  mutate(scenario = scenario_changer[scenario]) %>%
  ggplot() +
  geom_ribbon(aes(x=t,ymin=lq,ymax=uq,fill=variable),alpha=.5) +
  geom_line(aes(x=t,y=`Change in Cases`,color=variable)) +
  facet_grid(type~scenario) +
  theme_bw() + 
  theme(legend.position="none")

plt_box_intervene = final_size(output) %>%
  group_by(trial) %>%
  do({tmp = . ; tmp$final_size = .$final_size - .[.$scenario=='None_None',]$final_size; tmp}) %>%
  filter(scenario != 'None_None') %>%
  mutate(`Cases Averted` = -final_size,Intervention=scenario_changer[scenario]) %>%
  ggplot() +
  geom_boxplot(aes(x=Intervention, y=`Cases Averted`)) +
  ylim(c(-100,1000)) +
  theme_bw() + 
  geom_abline(slope=0)

plt_box_no_intervene = final_size(output) %>%
  group_by(trial) %>%
  mutate(`Final Size` = final_size,Intervention=scenario_changer[scenario]) %>%
  ggplot() +
  geom_boxplot(aes(x=Intervention, y=`Final Size`)) +
  theme_bw() + 
  ylim(c(2000,3200))
  

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
pdf('figures/epicurve.pdf')
print(plt)
dev.off()
pdf('figures/intervention-effects-final-size.pdf')
print(plot_inference(final_size,'Final_Size'))
dev.off()
pdf('figures/intervention-effects-peak-time.pdf')
print(plot_inference(peak_time,'Peak_Time'))
dev.off()
pdf('figures/intervention-effects-relative-risk.pdf')
print(plot_inference(final_size,'Log_Relative_Risk'))
dev.off()
pdf('figures/intervention-effects-time-series-recovered-switched.pdf')
print(plt_rec_t)
dev.off()
pdf('figures/intervention-effects-time-series-recovered.pdf')
print(plt_rec_t)
dev.off()
pdf('figures/intervention-effects-time-series-susceptible-switched.pdf')
print(plt_sus_t)
dev.off()
pdf('figures/intervention-effects-time-series-susceptible.pdf')
print(plt_sus)
dev.off()
pdf('figures/intervention-effects-raw-boxplots.pdf')
print(plt_box_no_intervene)
dev.off()
pdf('figures/intervention-effects-combined-boxplots.pdf')
print(plt_box_intervene)
while(dev.off() != 1){}
single_world_ci = confidence_intervals(final_size,'final_size') %>% filter(type=='single_world') %>% mutate(scenario = scenario_changer[scenario]) %>% arrange(scenario)
multiple_world_ci = confidence_intervals(final_size,'final_size') %>% filter(type=='multi_world') %>% mutate(scenario = scenario_changer[scenario]) %>% arrange(scenario)
paste0(
  "For single-world inference, we found that every non-null intervention had a significant (p<.05) effect on final size: ",
  paste(
    single_world_ci$scenario,
    "$",
    single_world_ci$final_size_m,
    "$ (CI $",
    single_world_ci$final_size_l,
    "$ --- $",
    single_world_ci$final_size_h,
    "$)",
    collapse=', '
  ),".")
paste0(
  "For multiple-world inference, we found that every all but one non-null intervention had a significant (p<.05) effect on final size: ",
  paste(
    multiple_world_ci$scenario,
    "$",
    multiple_world_ci$final_size_m,
    "$ (CI $",
    multiple_world_ci$final_size_l,
    "$ --- $",
    multiple_world_ci$final_size_h,
    "$)",
    collapse=', '
  ),".")

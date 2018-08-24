# try({remove.packages('counterfactual')},silent=T)
# install.packages('package',type='source',repos=NULL)
library(counterfactual)
library(ggplot2)
library(dplyr)
library(tidyr)

output = read_scenario('output/figures0')
multiworld_output = read_scenario('output/figures1')

plt = ggplot(output) + geom_point(aes(x=t,y=V1,color=paste(beta_name,susceptible_name)))
print(plt)

## Figure 1 - Illustration of the Problem
### Simulated Epidemic Curves with and without intervention
    # color intervention
plt = ggplot(output) + geom_point(aes(x=t,y=V2,color=paste(beta_name,susceptible_name)))
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
  if(name == 'Relative_Risk'){
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
      mutate_(.dots = setNames(paste(var_names,' / null'),var_names)) %>%
      gather_('scenario',name,var_names)
  } else {
    all_inference = all_inference %>%
      mutate_(.dots = setNames(paste(var_names,' - null'),var_names)) %>%
      gather_('scenario',name,var_names)
  }
  return(all_inference)
}

confidence_intervals = function(fun,name){
  tmp = all_world_inference(fun,name)
  tmp %>%
    group_by(type,scenario) %>%
    summarize_(.dots = setNames(c(paste0('quantile(',name,',.025)'), paste0('quantile(',name,',.975)'), paste0('mean(',name,')')),paste(name,c('l','h','m'),sep='_'))) %>%
    mutate_(.dots = setNames(paste0(name,'_h - ', name,'_l'),'width')) %>%
    return()
  
}

plot_inference = function(fun,name){
  x = all_world_inference(fun,name)
  x$type = gsub('_',' ',x$type)
  x$scenario = scenario_changer[x$scenario]
  plt = ggplot(x) +
    geom_boxplot(aes(x=scenario,color=type,y = x[[name]])) +
    scale_color_brewer(type='qual',palette='Paired')
  return(plt)
}

#### Peak Estimation
pdf('figures/intervention-effects-final-size.pdf')
print(plot_inference(final_size,'Final_Size'))
dev.off()
pdf('figures/intervention-effects-peak-time.pdf')
print(plot_inference(peak_time,'Peak_Time'))
dev.off()
pdf('figures/intervention-effects-relative-risk.pdf')
print(plot_inference(final_size,'Relative_Risk'))
while(dev.off() != 1){}

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

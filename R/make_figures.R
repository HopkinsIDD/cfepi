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

#### Final Size
plt2 <- output %>%
  filter(t == max(t))  %>%
  calculate_residual() %>%
  gather('scenario','value',None_None,None_Single,None_Constant,Flat_None) %>% 
  spread(variable,value) %>%
  ggplot() +
  geom_boxplot(aes(x = scenario,y=V3))

#### Peak Estimation
reshaped_output <- output %>%
  group_by(beta_name,susceptible_name,trial) %>%
  do({
    tmp = .
    tmp$max_V2 = max(.$V2)
    tmp
  }) %>%
  ungroup() %>%
  filter(V2 == max_V2)  %>%
  select(t,beta_name,susceptible_name,trial) %>% 
  unite(scenario,beta_name,susceptible_name) %>%
  spread(scenario,t)

plt3 <- reshaped_output %>% 
  mutate_if(
    !colnames(reshaped_output) %in% c('trial','t'),
    funs(as.numeric(.) - as.numeric(None_None))
  ) %>% 
  gather('scenario','value',None_None,None_Single,None_Constant,Flat_None) %>% 
  ggplot() +
  geom_boxplot(aes(x = scenario,y=value))

pdf('figures/intervention-effects-final-size.pdf')
print(plt2)
dev.off()
pdf('figures/intervention-effects-peak-estimation.pdf')
print(plt3)
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
    N = sum(output[1,c('V1','V2','V3','V4')],na.rm=T)

    lhs = output %>% filter(t == max(t)) %>% select(trial,V3,beta_name,susceptible_name)
    rhs = multiworld_output %>% filter(t == max(t)) %>% select(trial,V3)
    multiworld_output = inner_join(lhs,rhs,by='trial')
    multi_world_inference = multiworld_output %>% unite(scenario,beta_name,susceptible_name) %>% spread(scenario,V3.x) %>% mutate_if(c(F,F,T,T,T,T),funs(. / V3.y)) %>% gather('scenario','relative_risk',None_None,None_Single,None_Constant,Flat_None) %>% select(-V3.y)
    single_world_inference = multiworld_output %>% unite(scenario,beta_name,susceptible_name) %>% spread(scenario,V3.x) %>% mutate_if(c(F,F,T,T,F,T),funs(. / None_None)) %>% mutate(None_None = None_None / None_None) %>% gather('scenario','relative_risk',None_None,None_Single,None_Constant,Flat_None) %>% select(-V3.y)
    all_inference = inner_join(single_world_inference,multi_world_inference,by=c('scenario','trial'))
    names(all_inference)[3:4] = c('single_world','multi_world')
    all_inference = gather(all_inference,'type','relative_risk',single_world,multi_world)

    plt4 <- all_inference %>%
      ggplot() +
      geom_boxplot(aes(x = scenario,y=relative_risk,color=type))



pdf('figures/single-vs-multiple.pdf')
print(plt4)
while(dev.off() != 1){}
### Table 1 - Computational Resource Thresholds + Time

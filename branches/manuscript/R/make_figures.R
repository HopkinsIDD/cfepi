try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
library(counterfactual)
library(ggplot2)
library(dplyr)

output = read_scenario('output/figures')

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
  ggplot() +
  geom_boxplot(aes(x = paste(beta_name,susceptible_name),y=V3))

#### Peak Estimation
plt3 <- output %>%
  group_by(trial,beta_name,susceptible_name) %>%
  do({
    tmp = .
    tmp$max_V2 = max(.$V2)
    tmp
  }) %>% 
  ungroup() %>% 
  filter(V2 == max_V2)  %>%
  ggplot() +
  geom_boxplot(aes(x = paste(beta_name,susceptible_name),y=t))

pdf('figures/intervention-effects.pdf')
print(plt2)
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
    plt4 <- output %>%
      filter(V2 == max(V2))  %>%
      group_by(beta_name,susceptible_name) %>%
      summarize(
        relative_risk = V3/(V1 + V3)
      ) %>% 
      ggplot() +
      geom_boxplot(aes(x = paste(beta_name,susceptible_name),y=relative_risk))

pdf('figures/single-vs-multiple.pdf')
print(plt4)
dev.off()
### Table 1 - Computational Resource Thresholds + Time

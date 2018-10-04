# Define relevent directories
working_directory = rprojroot::find_root(rprojroot::has_file('.root')) # The location of the repository
setwd(working_directory)

try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
# try({remove.packages('counterfactual')},silent=T)
# install.packages('package',type='source',repos=NULL)

## Some parameters:
ci_width = .9
alpha = 1-ci_width


if(!require(counterfactual)){
  source("package/R/read.R")
}
# library(counterfactual)
library(cowplot)
library(ggplot2)
library(dplyr)
library(tidyr)
library(grid)

output = read_scenario('output/figures0')
output = as_data_frame(output)
residuals = calculate_residual(output)

scenario_changer = as.factor(c(
  'None_None' = 'No Intervention',
  'None_Constant' = 'Antivirals',
  'Flat_None' = 'Hand Washing',
  'None_Single' = 'Vaccination'
))

scenario_changer = relevel(scenario_changer,3,1,2,4)

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

output$scenario = scenario_changer[output$scenario]
output$variable = variable_changer[output$variable]
output$type = 'Single-World'
residuals$scenario = scenario_changer[residuals$scenario]
residuals$variable = variable_changer[residuals$variable]
residuals$type = 'Single-World'

#This provides mw_output and mw_residuals
source("R/multiple_world_time_trial.R")
mw_output = mw_output[names(output)]
mw_residuals = mw_residuals[names(residuals)]
all_world_residuals = rbind(residuals,mw_residuals)
all_world_output = rbind(output,mw_output)

all_world_output_summary = all_world_output %>%
  group_by(scenario,t,variable,type) %>%
  summarize(
    lq = quantile(value,alpha/2,na.rm=T),
    mean = mean(value,na.rm=T),
    hq = quantile(value,1-alpha/2,na.rm=T)
  )
all_world_residuals_summary = all_world_residuals %>%
  group_by(scenario,t,variable,type) %>%
  summarize(
    lq = quantile(value,alpha/2,na.rm=T),
    mean = mean(value,na.rm=T),
    hq = quantile(value,1-alpha/2,na.rm=T)
  )

confidence_intervals = all_world_residuals_summary %>%
  filter(t==100,variable=='R') %>%
  mutate(final_size_l = -hq, final_size_m = -mean, final_size_h = -lq) %>%
  ungroup %>%
  select(scenario,type,starts_with('final_size'))

type_to_opacity = c("Single-World" = .5, "Traditional" = .25)
plt_neg_rec_t = all_world_residuals_summary %>%
  filter(variable == 'R', t <= 100) %>%
  ungroup() %>%
  mutate(`Time (days)` = t, `Cases Averted` = -mean) %>% 
  ggplot() +
  geom_ribbon(aes(x=`Time (days)`,ymin=-lq,ymax=-hq,fill=type,alpha=type)) +
  geom_line(aes(x=`Time (days)`,y=`Cases Averted`,color=type,linetype=type,size=type)) +
  geom_abline(slope=0,intercept=0,linetype=2) +
  facet_grid(.~scenario) +
  theme_bw() + 
  # scale_alpha_discrete(limits=c(.5,.25)) +
  scale_size_discrete(range=c(1,.7)) +  
  scale_alpha_ordinal(range=c(.5,.26)) + 
  theme(aspect.ratio=1)

plt_box_output = output %>%
  filter(variable == 'R', t == 100) %>%
  ungroup() %>%
  mutate(`Cases` = value) %>% 
  ggplot() +
  geom_boxplot(aes(x=scenario,y=`Cases`)) +
  ylim(range=c(2250,3250))

plt_box_residuals = all_world_residuals %>%
  filter(variable == 'R', t == 100) %>%
  ungroup() %>%
  mutate(`Cases Averted` = -value) %>% 
  ggplot() +
  geom_boxplot(aes(x=scenario,y=`Cases Averted`,color=type)) +
  ylim(range=c(-500,600))


pdf('figures/intervention-effects-time-series-cases-averted-switched.pdf')
print(plt_neg_rec_t)
dev.off()
pdf('figures/intervention-effects-raw-boxplots.pdf')
print(plt_box_output)
dev.off()
pdf('figures/intervention-effects-combined-boxplots.pdf')
print(plt_box_residuals)
single_world_ci = confidence_intervals %>% filter(type=='Single-World')
multiple_world_ci = confidence_intervals %>% filter(type!='Single-World')
while(dev.off() != 1){}
ci_string_3 = paste0(
  "The single-world approach estimated ",
  tolower(single_world_ci$scenario),
  " to prevent an average of ",
  paste(
    "$",
    round(single_world_ci$final_size_m),
    "$ (",ci_width * 100,"\\% CI: $",
    ceiling(single_world_ci$final_size_l),
    "$\\textendash$",
    floor(single_world_ci$final_size_h),
    "$)"
  ),
  " cases, versus ",
  paste(
    "$",
    round(multiple_world_ci$final_size_m),
    "$ (",ci_width * 100,"\\% CI: $",
    ceiling(multiple_world_ci$final_size_l),
    "$\\textendash$",
    floor(multiple_world_ci$final_size_h),
    "$)"
  ),
  # c(" in the standard approach.",".",".",".") # For including no intervention
  c("."," in the standard approach.",".",".") # For not including no intervention
)
ci_string_3 = gsub(' -',' \\\\neg',ci_string_3)
cat(paste(paste(ci_string_3[c(2,3,4)],collapse='\n'),"\n"))

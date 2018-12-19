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
output = read_scenario(scenario = all_scenarios,intervention=all_interventions,ntrial=1000,path='output')
output = as_data_frame(output)

args = c(4000,100,10000,1.75,5,.1,1)
source("R/intervention_params.R")
scenario = scenario[grepl('main',scenario)]
tmp = read_scenario(scenario,intervention,10000,'output')
output = rbind(output,tmp)

args = c(40000,100,1000,1.75,5,.1,1)
source("R/intervention_params.R")
scenario = scenario[grepl('main',scenario)]
tmp = read_scenario(scenario,intervention,1000,'output')
output = rbind(output,tmp)

print("Last scenario read")

output$intervention = factor(output$intervention,levels=all_interventions)

residuals = calculate_residual(output)

output = filter(output,!is.na(value))
residuals = filter(residuals,!is.na(value))

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
residuals$variable = variable_changer[residuals$variable]
residuals$type = 'Single-World'
residuals$trial = as.numeric(residuals$trial)

tmprow = residuals[1,]
tmprow$value = NA
tmprow$variable='V'
residuals = rbind(residuals,tmprow)

tmprow = output[1,]
tmprow$value = NA
tmprow$variable='V'
output= rbind(output,tmprow)

all_mw_residuals = data_frame()
all_mw_output = data_frame()
for(ro in all_R0){
  for(ii in all_ii){
    for(prop in all_prop){
      for(t in all_start_time){
        args = c(4000,100,1000,ro,ii,prop,t)
        source("R/multiple_world_time_trial.R")
        if(nrow(all_mw_output)>0){
          all_mw_output = rbind(all_mw_output,mw_output)
          all_mw_residuals = rbind(all_mw_residuals,mw_residuals)
        } else {
          all_mw_output = mw_output
          all_mw_residuals = mw_residuals
        }
      }
    }
  }
}
# all_mw_residuals = foreach(ro = all_R0,.combine = 'rbind') %:%
#   foreach(ii = all_ii,.combine = 'rbind') %:%
#   foreach(prop = all_prop,.combine = 'rbind') %:%
#   foreach(t = all_start_time,.combine = 'rbind') %do% {
#     print(scenario)
#     args = c(4000,100,1000,ro,ii,prop,t)
#     source("R/multiple_world_time_trial.R")
#     tmp = mw_residuals
#     tmp$args = paste(args,sep='-')
#     tmp
#   }

# HERE 
args = c(4000,100,10000,1.75,5,.1,1)
source("R/multiple_world_time_trial.R")
all_mw_output = rbind(all_mw_output,mw_output)
all_mw_residuals = rbind(all_mw_residuals,mw_residuals)
args = c(40000,100,1000,1.75,5,.1,1)
source("R/multiple_world_time_trial.R")
all_mw_residuals = rbind(all_mw_residuals,mw_residuals)
all_mw_output = rbind(all_mw_output,mw_output)

all_mw_residuals = filter(all_mw_residuals,intervention != 'Uncontrolled')
all_mw_output = filter(all_mw_output,intervention != 'Uncontrolled')

all_mw_output$scenario = gsub('.','',all_mw_output$scenario,fixed=TRUE)
all_mw_residuals$scenario = gsub('.','',all_mw_residuals$scenario,fixed=TRUE)

all_mw_output$intervention = gsub('.','',all_mw_output$intervention,fixed=TRUE)
all_mw_residuals$intervention = gsub('.','',all_mw_residuals$intervention,fixed=TRUE)

all_mw_output$intervention = factor(all_mw_output$intervention,levels=all_interventions)
all_mw_residuals$intervention = factor(all_mw_residuals$intervention,levels=all_interventions)

all_mw_output = all_mw_output[names(output)]
all_mw_residuals = all_mw_residuals[names(residuals)]
all_world_residuals = rbind(residuals,all_mw_residuals)
all_world_output = rbind(output,all_mw_output)

### Things
# all_world_output = all_world_output %>% mutate(scenario = gsub('main','',scenario))
all_world_output = all_world_output %>% mutate(scenario = substring(scenario,first=5))
all_world_output = separate(all_world_output,scenario,c('population','ntime','ntrial','R0','ninf'))
all_world_residuals = all_world_residuals %>% mutate(scenario = substring(scenario,first=5))
all_world_residuals = separate(all_world_residuals,scenario,c('population','ntime','ntrial','R0','ninf'))

all_world_output = all_world_output %>% group_by(intervention) %>% do({
  tmp = .;
  if(tmp$intervention[[1]] == 'No Intervention'){
    # Do nothing
  } else if(substring(tmp$intervention[[1]],1,10) == 'Antivirals'){
    # tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','coverage','rate','to','from'))
    tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','coverage','rate','to','from'))
    tmp$coverage = gsub('^0','0.',tmp$coverage)
    tmp$rate = gsub('^0','0.',tmp$rate)
  } else if(substring(tmp$intervention[[1]],1,11) == 'Vaccination'){
    # tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate','to','from','coverage'))
    tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate','to','from'))
    tmp$rate = gsub('^0','0.',tmp$rate)
  } else if(substring(tmp$intervention[[1]],1,12) == 'Hand Washing'){
    # tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate','to','from','coverage'))
    tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate'))
    tmp$rate = gsub('^0','0.',tmp$rate)
  }
  tmp
})
all_world_output = mutate(all_world_output,coverage=as.numeric(coverage),rate=as.numeric(rate),tstart=as.numeric(tstart))

all_world_residuals = all_world_residuals %>% group_by(intervention) %>% do({
  tmp = .;
  if(tmp$intervention[[1]] == 'No Intervention'){
    # Do nothing
  } else if(substring(tmp$intervention[[1]],1,10) == 'Antivirals'){
    # tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','coverage','rate','to','from'))
    tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','coverage','rate','to','from'))
    tmp$coverage = gsub('^0','0.',tmp$coverage)
    tmp$rate = gsub('^0','0.',tmp$rate)
  } else if(substring(tmp$intervention[[1]],1,11) == 'Vaccination'){
    # tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate','to','from','coverage'))
    tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate','to','from'))
    tmp$rate = gsub('^0','0.',tmp$rate)
  } else if(substring(tmp$intervention[[1]],1,12) == 'Hand Washing'){
    # tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate','to','from','coverage'))
    tmp = separate(tmp,intervention,sep='-',into = c('intervention','tstart','rate'))
    tmp$rate = gsub('^0','0.',tmp$rate)
  }
  tmp
})
all_world_residuals = mutate(all_world_residuals,coverage=as.numeric(coverage),rate=as.numeric(rate),tstart=as.numeric(tstart))

# all_world_output = spread(all_world_output


all_world_output_summary = all_world_output %>%
  group_by(
    population,ntime,ntrial,R0,ninf, # scenario
    intervention,tstart,coverage,rate,to,from, #intervention
    t,
    variable,
    type
  ) %>% 
  summarize(
    lq = quantile(value,alpha/2,na.rm=T),
    mean = mean(value,na.rm=T),
    median = median(value,na.rm=T),
    hq = quantile(value,1-alpha/2,na.rm=T)
  )
all_world_residuals_summary = all_world_residuals %>%
  group_by(
    population,ntime,ntrial,R0,ninf, # scenario
    intervention,tstart,coverage,rate,to,from, #intervention
    t,
    variable,
    type
  ) %>% 
  summarize(
    lq = quantile(value,alpha/2,na.rm=T),
    mean = mean(value,na.rm=T),
    median = median(value,na.rm=T),
    hq = quantile(value,1-alpha/2,na.rm=T)
  )

confidence_intervals = all_world_residuals_summary %>%
  filter(t==100,variable=='R') %>%
  mutate(final_size_l = -hq, final_size_m = -median, final_size_h = -lq) %>%
  ungroup %>%
  select(
    population,ntime,ntrial,R0,ninf, # scenario
    intervention,tstart,coverage,rate,to,from, #intervention
    type,
    starts_with('final_size')
  )

type_to_opacity = c("Single-World" = .5, "Traditional" = .25)

plt_neg_rec_t = all_world_residuals_summary %>%
# plt_neg_rec_t = all_world_output_summary %>%
  filter(ntrial == 10000, variable == 'R', t <= 100) %>%
  # filter(scenario == 'main4000-100-10000-175-5', variable == 'R', t <= 100) %>%
  ungroup() %>%
  mutate(`Time (days)` = t, `Cases Averted` = -mean) %>% 
  ggplot() +
  geom_ribbon(aes(x=`Time (days)`,ymin=-lq,ymax=-hq,fill=type,alpha=type)) +
  geom_line(aes(x=`Time (days)`,y=`Cases Averted`,color=type,linetype=type,size=type)) +
  geom_abline(slope=0,intercept=0,linetype=2) +
  facet_wrap(~intervention) +
  theme_bw() + 
  # scale_alpha_discrete(limits=c(.5,.25)) +
  scale_size_discrete(range=c(1,.7)) +  
  scale_alpha_ordinal(range=c(.5,.26)) + 
  theme(aspect.ratio=1)


pdf('figures/intervention-effects-time-series-cases-averted-switched.pdf')
print(plt_neg_rec_t)
dev.off()

# plt_box_output = output %>%
#   filter(variable == 'R', t == 100) %>%
#   ungroup() %>%
#   mutate(`Cases` = value) %>% 
#   ggplot() +
#   geom_boxplot(aes(x=scenario,y=`Cases`)) +
#   ylim(range=c(2250,3250))
# 
# plt_box_residuals = all_world_residuals %>%
#   filter(variable == 'R', t == 100) %>%
#   ungroup() %>%
#   mutate(`Cases Averted` = -value) %>% 
#   ggplot() +
#   geom_boxplot(aes(x=scenario,y=`Cases Averted`,color=type)) +
#   ylim(range=c(-500,600))
# pdf('figures/intervention-effects-raw-boxplots.pdf')
# print(plt_box_output)
# dev.off()
# pdf('figures/intervention-effects-combined-boxplots.pdf')
# print(plt_box_residuals)
single_world_ci = confidence_intervals %>% filter(type=='Single-World',scenario == 'main4000-100-10000-175-5')
multiple_world_ci = confidence_intervals %>% filter(type!='Single-World',scenario == 'main4000-100-10000-175-5')
while(dev.off() != 1){}
ci_string_3 = paste0(
  "The single-world approach estimated ",
  tolower(single_world_ci$intervention),
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
cat(paste(paste(ci_string_3[c(2,1,4)],collapse='\n'),"\n"))


library(crop)
library(dplyr)
library(ggplot2)
library(tidyr)

# output = readRDS('all_world_output_summary.rds')
# residuals = readRDS('all_world_residuals_summary.rds')

residuals %>%
  ungroup() %>% 
  filter(t == 100, variable == 'R') %>%
  mutate(
    scenario = paste(population,ntrial,ninf,as.numeric(R0)/100),
    intervention_params = paste(intervention,tstart,as.numeric(coverage),as.numeric(rate)),
    significance = hq < 0
  ) %>%
  select(scenario,intervention_params,type,significance) %>% 
  spread(type,significance) -> tmp
scenario_numberer = setNames(factor(paste("Scenario",1:length(unique(tmp$scenario))),levels = paste("Scenario",1:length(unique(tmp$scenario))))
                             ,sort(unique(tmp$scenario)))
intervention_numberer = setNames(factor(paste("Intervention",1:length(unique(tmp$intervention_params))),levels = paste("Intervention",1:length(unique(tmp$intervention_params))))
                                 ,sort(unique(tmp$intervention_params)))
plt = tmp%>%
  mutate(
    scenario = scenario_numberer[scenario],
    intervention_params = intervention_numberer[intervention_params],
    value = ifelse(
      `Single-World`,
      ifelse(Traditional,
             'Both',
             'Just Single World'),
      ifelse(Traditional,
             'Just Traditional',
             'Neither'
      )
    )
  ) %>%
  ggplot(aes(y=scenario,x=intervention_params,fill=value)) + 
  geom_tile(color='black') +
  labs(fill="Models with Significant Results",x='Intervention',y='Scenario') + 
  theme_bw() + 
  theme(axis.text.x = element_text(angle = 90, hjust = 1))
# theme_bw()

pdf('figures/stability-significance-all.pdf')
print(plt)
dev.off()

residuals %>%
  ungroup() %>% 
  filter(t == 100, variable == 'R') %>%
  mutate(
    scenario = paste(population,ntrial,ninf,as.numeric(R0)/100),
    intervention_params = paste(intervention,tstart,as.numeric(coverage),as.numeric(rate)),
    significance = hq > 0
  ) %>%
  select(scenario,intervention_params,type,significance) %>% 
  spread(type,significance) -> tmp
scenario_numberer = setNames(factor(paste("Scenario",1:length(unique(tmp$scenario))),levels = paste("Scenario",1:length(unique(tmp$scenario))))
                             ,sort(unique(tmp$scenario)))
intervention_numberer = setNames(factor(paste("Intervention",1:length(unique(tmp$intervention_params))),levels = paste("Intervention",1:length(unique(tmp$intervention_params))))
                                 ,sort(unique(tmp$intervention_params)))
plt = tmp%>%
  mutate(
    scenario = scenario_numberer[scenario],
    intervention_params = intervention_numberer[intervention_params],
    value = ifelse(
      `Single-World`,
      ifelse(Traditional,
             'Both',
             'Just Single World'),
      ifelse(Traditional,
             'Just Traditional',
             'Neither'
      )
    )
  ) %>%
  ggplot(aes(y=scenario,x=intervention_params,fill=value)) + 
  geom_tile(color='black') +
  labs(fill="Models with Negative Results",x='Intervention',y='Scenario') + 
  theme_bw() + 
  theme(axis.text.x = element_text(angle = 90, hjust = 1))
# theme_bw()

pdf('figures/stability-weirdness-all.pdf')
print(plt)
dev.off()

tmp = output %>% filter(variable =='I')
counter=0
pdf('~/tmp.pdf')
while(T){
  counter = counter + 1
  plt = tmp %>%
    ungroup %>%
    # filter(intervention == "No Intervention",population == 4000,ntime == 100,ntrial == 1000,R0==125,ninf==10) %>% 
    ggplot(aes(x=t,color=type,fill=type)) +
    geom_line(aes(linetype=type,y=mean,size=type)) +
    # geom_ribbon(aes(ymin=lq,ymax=hq,alpha=type)) +
    ggforce::facet_grid_paginate(paste(population,ntime,ntrial,R0,ninf) ~ paste(intervention,tstart,coverage,rate,to,from),nrow=3,ncol=3,page=counter,scales = 'free') +
    # theme_bw() +
    # scale_alpha_discrete(limits=c(.5,.25)) +
    scale_size_discrete(range=c(1,1.7)) +
    scale_alpha_ordinal(range=c(.25,.126)) +
    theme(aspect.ratio=1)
  print(plt)
}
dev.off()

tmp = residuals %>% filter(variable =='R')
signedlog_10 = function(x){
  return(sign(x) * log(abs(x) + 1)/log(10))
}
counter=0
pdf('figures/residual_curves.pdf')
while(T){
  counter = counter + 1
  plt = tmp %>%
    ungroup %>%
    # filter(intervention == "No Intervention",population == 4000,ntime == 100,ntrial == 1000,R0==125,ninf==10) %>% 
    ggplot(aes(x=t,color=type,fill=type)) +
    geom_line(aes(linetype=type,y=signedlog_10(-mean),size=type)) +
    geom_ribbon(aes(ymin=signedlog_10(-hq),ymax=signedlog_10(-lq),alpha=type)) +
    ggforce::facet_grid_paginate(paste(population,ntime,ntrial,R0,ninf) ~ paste(intervention,tstart,coverage,rate,to,from),nrow=2,ncol=2,page=counter,scales = 'free') +
    geom_hline(aes(yintercept=0,linetype = 'zero')) + 
    theme_bw() +
    # scale_alpha_discrete(limits=c(.5,.25)) +
    scale_size_discrete(range=c(1,1.7)) +
    scale_alpha_ordinal(range=c(.25,.126)) +
    theme(aspect.ratio=1)
  print(plt)
}
dev.off()


for(variable )
residuals %>% 
  ungroup %>%
  filter(t==100,variable=='R') %>%
  mutate(significance = hq > 0) %>%
  select(-lq,-mean,-median,-hq) %>%
  spread(type,significance) %>%
  mutate(
    value = ifelse(
      `Single-World`,
      ifelse(Traditional,
             'Both',
             'Just Single World'),
      ifelse(Traditional,
             'Just Traditional',
             'Neither'
      )
    )
  ) %>%
  ggplot(aes(x=R0,y=value,fill=value)) +
  geom_bar() + 
  scale_y_continuous(labels = scales::percent)

for(variable in c('population','R0','ninf','tstart','coverage')){
  plt = residuals %>% 
    ungroup %>%
    filter(t==100,variable=='R') %>%
    mutate(significance = hq > 0) %>%
    select(-lq,-mean,-median,-hq) %>%
    spread(type,significance) %>%
    mutate(
      value = ifelse(
        `Single-World`,
        ifelse(Traditional,
               'Both',
               'Just Single World'),
        ifelse(Traditional,
               'Just Traditional',
               'Neither'
        )
      )
    ) %>% 
    group_by_(variable,'value') %>%
    tally() %>%
    ungroup() %>%
    group_by_(variable) %>%
    mutate(weight = n / sum(n)) %>%
    ggplot(aes(x=population,weight=weight,fill=value)) +
    geom_bar() +
    theme(legend.title = element_text("Hello")) +
    labs(fill="Models with Significant Results",x=tools::toTitleCase(variable),y='Percent across other variables')
  print(plt)
}

variable_changer = setNames(c('Population','R0','Initial Infected','Start Time','Coverage','Number of Trials'),c('population','R0','ninf','tstart','coverage','ntrial'))
for(var in names(variable_changer)){
  filename = paste('figures/stability',var,'significance.pdf',sep='-')
  pdf(filename)
  plt = residuals %>% 
    ungroup %>%
    filter(t==100,variable=='R') %>%
    mutate(significance = hq < 0) %>%
    select(-lq,-mean,-median,-hq) %>%
    spread(type,significance) %>%
    mutate(
      R0 = as.numeric(R0) / 100,
      ninf = as.numeric(ninf)
    ) %>%
    mutate_(.dots=setNames(var,'variable')) %>%
    mutate(
      value = factor(
        ifelse(
          `Single-World`,
          ifelse(Traditional,
                 'Both',
                 'Just Single World'),
          ifelse(Traditional,
                 'Just Traditional',
                 'Neither'
          )
        ),
        levels = c('Neither','Just Traditional','Both','Just Single World')
      )
    ) %>%
    group_by(variable,value) %>%
    tally() %>%
    ungroup() %>%
    group_by(variable) %>%
    mutate(weight = n / sum(n)) %>%
    ggplot(aes(x=factor(variable,levels=as.character(sort(as.numeric(unique(variable))))),weight=weight,fill=value)) +
    geom_bar() +
    # theme(legend.title = element_text("Hello")) +
    labs(fill="Models with Significant Results",x=variable_changer[var],y='Percent across other variables') + 
    theme_bw() + 
    theme(aspect.ratio=1)
  print(plt)
  dev.off.crop()
}


for(var in names(variable_changer)){
  filename = paste('figures/stability',var,'weirdness.pdf',sep='-')
  pdf(filename)
  plt = residuals %>% 
    ungroup %>%
    filter(t==100,variable=='R') %>%
    mutate(significance = hq > 0) %>%
    select(-lq,-mean,-median,-hq) %>%
    spread(type,significance) %>%
    mutate(
      R0 = as.numeric(R0) / 100,
      ninf = as.numeric(ninf)
    ) %>%
    mutate_(.dots=setNames(var,'variable')) %>%
    mutate(
      value = factor(
        ifelse(
          `Single-World`,
          ifelse(Traditional,
                 'Both',
                 'Just Single World'),
          ifelse(Traditional,
                 'Just Traditional',
                 'Neither'
          )
        ),
        levels = c('Neither','Just Traditional','Both','Just Single World')
      )
    ) %>%
    group_by(variable,value) %>%
    tally() %>%
    ungroup() %>%
    group_by(variable) %>%
    mutate(weight = n / sum(n)) %>%
    ggplot(aes(x=factor(variable,levels=as.character(sort(as.numeric(unique(variable))))),weight=weight,fill=value)) +
    geom_bar() +
    # theme(legend.title = element_text("Hello")) +
    labs(fill="Models with Negative Results",x=variable_changer[var],y='Percent across other variables') + 
    theme_bw() + 
    theme(aspect.ratio=1)
  print(plt)
  dev.off.crop()
}

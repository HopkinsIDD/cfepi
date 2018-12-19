options(warn=1)
options(scipen=999)
if(!('reinstalled' %in% ls())){
  options(warn=1)
  try({remove.packages('cfepi')},silent=T)
  install.packages('package',type='source',repos=NULL)
  library(cfepi)
  reinstalled =TRUE
}

## Params from arguments:
## npop : first argument
## ntime : second argument
## ntrial : third argument

npop = as.numeric(args[1])
ntime = as.numeric(args[2])
ntrial = as.numeric(args[3])
R0 = as.numeric(args[4]) # 1.75
ninf= as.numeric(args[5]) # 5

D = 2.25 #days
gamma = 1/D
beta = R0 * gamma

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - ninf,ninf,0)
inter <- inter/npop

# warning('Using old filenames')
# output_name1 = paste0('output/main-',gsub('.','',paste(format(args[1:5],scientific=FALSE),collapse='-'),fixed=T))
output_name1 = paste0('output/main',gsub('.','',paste(args[1:5],collapse='-'),fixed=T))
# output_name2 = paste0('output/secondworld-',gsub('.','',paste(format(args[1:5],scientific=FALSE),collapse='-'),fixed=T))
# output_name2 = paste0('output/secondworld',gsub('.','',paste(args[1:5],collapse='-'),fixed=T))

# scenario = gsub('output/','',c(output_name1,output_name2),fixed=TRUE)
scenario = gsub('output/','',c(output_name1),fixed=TRUE)

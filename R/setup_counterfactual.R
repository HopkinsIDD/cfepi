args = commandArgs(trailingOnly=TRUE)
try({remove.packages('cfepi')},silent=T)
install.packages('package',type='source',repos=NULL)

library(cfepi)

source("R/counterfactual_params.R")
# This code is commented out as it takes a long time to run, and need only run a single time
ptm <- proc.time()
setup_counterfactual(
  output_name1,
  init,
  inter,
  trans,
  ntime,
  ntrial
)
total_time <- (proc.time() - ptm)[3]
print(paste("It took",total_time,"seconds to set up counterfactual for arguments: ",paste(args,collapse = ' '),'and',ntrial,"trials."))

ptm <- proc.time()
# setup_counterfactual(
#   output_name2,
#   init,
#   inter,
#   trans,
#   ntime,
#   ntrial
# )
total_time <- (proc.time() - ptm)[3]
print(paste("It took",total_time,"seconds to set up counterfactual for arguments: ",paste(args,collapse = ' '),'and',ntrial,"trials."))

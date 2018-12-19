if(!('reinstalled' %in% ls())){
  args = commandArgs(trailingOnly=TRUE)
  try({remove.packages('cfepi')},silent=T)
  install.packages('package',type='source',repos=NULL)
  library(cfepi)
  reinstalled =TRUE
}

ptm <- proc.time()
source("R/setup_null_intervention.R")
total_time <- (proc.time() - ptm)[3]
print(paste("It took",total_time,"seconds to set up null intervention for arguments: ",paste(args,collapse = ' '),'and',ntrial,"trials."))

ptm <- proc.time()
source("R/setup_antiviral_intervention.R")
total_time <- (proc.time() - ptm)[3]
print(paste("It took",total_time,"seconds to set up antiviral intervention for arguments: ",paste(args,collapse = ' '),'and',ntrial,"trials."))

ptm <- proc.time()
source("R/setup_vaccination_intervention.R")
total_time <- (proc.time() - ptm)[3]
print(paste("It took",total_time,"seconds to set up vaccination intervention for arguments: ",paste(args,collapse = ' '),'and',ntrial,"trials."))

ptm <- proc.time()
source("R/setup_handwashing_intervention.R")
total_time <- (proc.time() - ptm)[3]
print(paste("It took",total_time,"seconds to set up hand washing intervention for arguments: ",paste(args,collapse = ' '),'and',ntrial,"trials."))

ptm <- proc.time()

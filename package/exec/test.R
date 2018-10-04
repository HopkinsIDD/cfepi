options(warn=1)
options(error=traceback)
install.packages('.',type='source',repos=NULL)
# suppressPackageStartupMessages()
library(counterfactual)

trans <- matrix(0,5,5)
inter <- matrix(0,5,5)
trans[2,1] <- .1
trans[1,2] <- .1
trans[3,4] <- .1
trans[4,3] <- .1
trans[5,4] <- .1
trans[5,3] <- .1
inter[3,1] <- .2
inter[4,2] <- .8
init <- c(39990,39,0,10,0)
# init <- c(390,10,0)
npop <- sum(init)
inter <- inter/npop
ntime <- 365
ntrial <- 1000
fname <- "output/test"

results <- counterfactual::run_full_counterfactual(
  fname,
  init,
  inter,
  trans,
  "Flat",
  "None",
  list(start_time = 10,rate = .1),
  list(),
  ntime,
  ntrial
)
# setup_counterfactual(fname,init,inter,trans,ntime,ntrial=ntrial)
# run_scenario(fname,init,"Flat","None",list(start_time=30,rate=.15),list(),ntime,ntrial)
# run_scenario(fname,init,"None","None",list(),list(),ntime,ntrial)
results <- read_scenario(fname)
residuals = calculate_residual(results)

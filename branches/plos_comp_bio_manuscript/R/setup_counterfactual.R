try({remove.packages('counterfactual')},silent=T)
install.packages('package',type='source',repos=NULL)
library(counterfactual)


R0 = 1.75 #
mu = 2.25 #days
gamma = 1/mu
beta = R0 * gamma
npop = 4000
ntime = 100
ntrial = 1000

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- gamma
inter[2,1] <- beta
init <- c(npop - 1,1,0)
inter <- inter/npop
# This code is commented out as it takes a long time to run, and need only run a single time
setup_counterfactual(
  'output/figures0',
  init,
  inter,
  trans,
  ntime,
  ntrial
)
setup_counterfactual(
  'output/figures1',
  init,
  inter,
  trans,
  ntime,
  ntrial
)

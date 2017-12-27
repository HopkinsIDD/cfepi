testfun <- function(){
  print("Hello World")
}
trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- .2
inter[2,1] <- .4
init <- c(3999990,10,0)
npop <- sum(init)
inter <- inter/npop
ntime <- 365
ntrial <- 1

# dyn.load('~/svn/counterfactual/branches/R_compliant/src/counterfactual.so')
# dyn.load('~/svn/counterfactual/branches/R_compliant/src/rinterface.so')
# .Call('setupCounterfactualAnalysis',"first_counterfactual",init,inter,trans,ntime,ntrial)
# dyn.unload('~/svn/counterfactual/branches/R_compliant/src/rinterface.so')
# dyn.unload('~/svn/counterfactual/branches/R_compliant/src/counterfactual.so')

dyn.load('src/counterfactual.dll')
.Call('setupCounterfactualAnalysis',
  "first_counterfactual",
  init,
  inter,
  trans,
  ntime,
  ntrial
)
.Call('runIntervention',
  "first_counterfactual",
  init,
  function(){}, 
  function(){},
  ntime, 
  ntrial
)
dyn.unload('src/counterfactual.dll')
results <- read.csv('output/first_counterfactual.noint.0.csv',header = FALSE)[,-4]

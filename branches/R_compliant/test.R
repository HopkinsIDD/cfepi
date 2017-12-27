testfun <- function(){
  print("Hello World")
}
trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[2,3] <- .1
inter[1,2] <- .2
init <- c(3990,10,0)
npop <- sum(init)
inter <- inter/npop
ntime <- 10
ntrial <- 1

dyn.load('~/svn/counterfactual/branches/R_compliant/src/counterfactual.so')
dyn.load('~/svn/counterfactual/branches/R_compliant/src/rinterface.so')
.Call('setupCounterfactualAnalysis',"first_counterfactual",init,inter,trans,ntime,ntrial)
dyn.unload('~/svn/counterfactual/branches/R_compliant/src/rinterface.so')
dyn.unload('~/svn/counterfactual/branches/R_compliant/src/counterfactual.so')

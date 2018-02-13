testfun <- function(){
  print("Hello World")
}
trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- .2
inter[2,1] <- .4
init <- c(3990,10,0)
# init <- c(390,10,0)
npop <- sum(init)
inter <- inter/npop
ntime <- 365
ntrial <- 1


# dyn.load('src/counterfactual.dll')
dyn.load('src/counterfactual.so')
dyn.load('src/rinterface.so')
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
  "None",
  "None",
  list(),
  list(),
  ntime, 
  ntrial
)
.Call('runIntervention',
  "first_counterfactual",
  init,
  "Flat",
  "None",
  list(start_time = 30,rate= .05),
  list(),
  ntime, 
  ntrial
)
# dyn.unload('src/counterfactual.dll')
dyn.unload('src/counterfactual.so')
dyn.unload('src/rinterface.so')
# results <- read.csv('output/first_counterfactual.None.None.0.csv',header = FALSE)[,-4]
gc()

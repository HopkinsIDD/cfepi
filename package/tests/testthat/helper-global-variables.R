# setup_counterfactual_working = FALSE
run_scenario_working = FALSE
cleanup_working = FALSE

trans <- matrix(0,3,3)
inter <- matrix(0,3,3)
trans[3,2] <- .2
init <- c(3990,10,0)
# init <- c(390,10,0)
npop <- sum(init)
inter <- inter/npop
ntime <- 365
ntrial <- 1

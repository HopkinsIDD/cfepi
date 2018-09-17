times = 1:100 * NA
for(trial in 1:100){
  initial = c(3999990,10,0)
  current = initial
  R0 = 1.75 #
  mu = 2.25 #days
  gamma = 1/mu
  beta = R0 * gamma/sum(initial)
  total = matrix(NA,3,366)
  Rprof()
  for(i in 1:365){
    total[,i] = current
    # Infections
    if(current[2] > 0){
      ninfected = 0
      for(j in 1:current[2]){
        ninfected = ninfected + rbinom(n=1,size=current[1]-ninfected,prob=beta)
      }
      nrecovered = rbinom(n=1,size=current[2],prob=gamma)
      current[1] = current[1] - ninfected
      current[2] = current[2] + ninfected - nrecovered
      current[3] = current[3] + nrecovered
      
    }
    
  }
  Rprof(NULL)
  tmp = summaryRprof()
  times[trial] = tmp$sampling.time
  # This needs to happen twice, once with and once without the intervention so multiply all times by 2.
}

testfun <- function(){
  print("Hello World")
}
dyn.load('~/svn/counterfactual/branches/R_compliant/src/test.so')
.Call('test',testfun)
dyn.unload('~/svn/counterfactual/branches/R_compliant/src/test.so')

testfun <- function(){
  print("Hello World")
}
dyn.load('src/test.so')
.Call('test',testfun)
dyn.unload('src/test.so')

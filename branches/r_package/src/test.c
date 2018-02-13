#include <R.h>
#include <Rmath.h>
#include <Rinternals.h>

SEXP test(SEXP fun){
  SEXP e;
  PROTECT(
    e = eval(
      lang1(
        fun
      ),
      R_GlobalEnv
    )
  );
  UNPROTECT(1);
  return(e);
}

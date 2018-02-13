#include <R.h>
#include <Rmath.h>
#include <Rinternals.h>
#include <R_ext/Print.h>
#include <Rdefines.h>
#include "rinterface.h"
#include "counterfactual.h"
#include "all_interventions.h"

SEXP setupCounterfactualAnalysis(SEXP Rfilename, SEXP RinitialConditions, SEXP Rinteractions, SEXP Rtransitions, SEXP Rntime, SEXP Rntrial){
  double* interactions;
  double* transitions;
  double* output;
  SEXP Routput;
  int* init;
  int nvar,nvar1,nvar2,ntime,trial,ntrial;
  char* filename;
  char ifn[1000];
  char tfn[1000];
  int var,var2;

  GetRNGstate();
  
  R2cstring(Rfilename,&filename);
  R2cvecint(RinitialConditions,&init,&nvar);
  REprintf("First Call\n");
  ntime = R2cint(Rntime);
  REprintf("Second Call\n");
  ntrial = R2cint(Rntrial);
  R2cmat(Rtransitions,&transitions,&nvar1,&nvar2);
  if(nvar1 != nvar){
    REprintf("Dimension mismatch.  The transition matrix should have one row for each variable.\n");
    return(R_NilValue);
  }
  if(nvar2 != nvar){
    REprintf("Dimension mismatch.  The transition matrix should have one col for each variable.\n");
    return(R_NilValue);
  }
  R2cmat(Rinteractions,&interactions,&nvar1,&nvar2);
  if(nvar1 != nvar){
    REprintf("Dimension mismatch.  The interaction matrix should have one row for each variable.\n");
    return(R_NilValue);
  }
  if(nvar2 != nvar){
    REprintf("Dimension mismatch.  The interaction matrix should have one col for each variable.\n");
    return(R_NilValue);
  }

  //Things are loaded now

  for(trial = 0; trial < ntrial; ++trial){
    Rprintf("Running Trial %d\n",trial);
    sprintf(ifn,"output/%s.i.0.%d.csv",filename,trial);
    sprintf(tfn,"output/%s.t.0.%d.csv",filename,trial);
    Rprintf("init:");
    for(var = 0; var < nvar; ++var){
      Rprintf(" %d",init[var]);
    }
    Rprintf("\nnvar: %d\nntime %d\ntransitions:\n",nvar,ntime);
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        Rprintf(" %f",transitions[IND(var,var2,nvar)]);
      }
      Rprintf("\n");
    }
    Rprintf("\ninteractions:\n");
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        Rprintf(" %f",interactions[IND(var,var2,nvar)]);
      }
      Rprintf("\n");
    }
    Rprintf("\ntfn: %s\nifn: %s\n",tfn,ifn);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
    // sprintf(ifn,"output/%s.i.1.%d.csv",trial);
    // sprintf(tfn,"output/%s.t.1.%d.csv",trial);
    // runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
  }
  
  //Cleanup starts here

  PutRNGstate();
  return(R_NilValue);
}

SEXP runIntervention(SEXP Rfilename, SEXP RinitialConditions, SEXP RreduceBeta, SEXP ReliminateSusceptibles, SEXP RbetaPars, SEXP RsusceptiblePars, SEXP Rntime, SEXP Rntrial){
  double* output;
  SEXP Routput;
  int* init;
  int nvar,nvar1,nvar2,ntime,trial,ntrial;
  char* filename;
  char* reduceBeta_name;
  char* eliminateSusceptibles_name;
  char ifn[1000];
  char tfn[1000];
  char fn[1000];
  int var,var2;

  beta_t intervention_unparametrized_reduceBeta;
  param_beta_t param_beta;
  saved_beta_t intervention_reduceBeta;

  susceptible_t intervention_unparametrized_eliminateSusceptibles;
  param_susceptible_t param_susceptible;
  saved_susceptible_t intervention_eliminateSusceptibles;

  GetRNGstate();
  
  R2cstring(Rfilename,&filename);
  R2cvecint(RinitialConditions,&init,&nvar);
  REprintf("Third Call\n");
  ntime = R2cint(Rntime);
  REprintf("Fourth Call\n");
  ntrial = R2cint(Rntrial);
  //These may change later
  R2cstring(RreduceBeta,&reduceBeta_name);
  R2cstring(ReliminateSusceptibles,&eliminateSusceptibles_name);
 
  //Things are loaded now
  //Choose the type of intervention:
  if(strcmp(reduceBeta_name,"None")==0){
    intervention_unparametrized_reduceBeta = &no_beta;
    param_beta = param_no_beta();
  } else if(strcmp(reduceBeta_name,"Flat")==0){
    intervention_unparametrized_reduceBeta = &flat_beta;
    REprintf("Fifth Call\n");
    Rprintf("length of pars is %d\n",LENGTH(RbetaPars));
    fflush(stdout);
    Rprintf("length of pars[[1]] is %d\n",LENGTH(VECTOR_ELT(RbetaPars,0)));
    Rprintf("pars[[1]] is %d\n",R2cint(VECTOR_ELT(RbetaPars,0)));
    fflush(stdout);
    Rprintf("length of pars[[2]] is %d\n",LENGTH(VECTOR_ELT(RbetaPars,0)));
    Rprintf("pars[[2]] is %f\n",R2cdouble(VECTOR_ELT(RbetaPars,1)));
    fflush(stdout);
    param_beta = param_flat_beta(
      R2cint(VECTOR_ELT(RbetaPars,0)),
      R2cdouble(VECTOR_ELT(RbetaPars,1))
    );
  } else {
    Rprintf("Could not recognize the beta specification %s\n",reduceBeta_name);
    return(R_NilValue);
  }
  if(strcmp(eliminateSusceptibles_name,"None")==0){
    intervention_unparametrized_eliminateSusceptibles = &no_susceptible;
    param_susceptible = param_no_susceptible();
  } else if (strcmp(eliminateSusceptibles_name,"Single")==0){
    REprintf( "This code is not yet written\n");
    exit(1);
    intervention_unparametrized_eliminateSusceptibles = &flat_susceptible;
    param_susceptible = param_no_susceptible();
  } else {
    Rprintf("Could not recognize the susceptibles specification %s\n",eliminateSusceptibles_name);
    return(R_NilValue);
  }

  //Set the parameters (This should eventually be based on the R input)
  intervention_reduceBeta = partially_evaluate_beta(intervention_unparametrized_reduceBeta,param_beta);
  intervention_eliminateSusceptibles = partially_evaluate_susceptible(intervention_unparametrized_eliminateSusceptibles,param_susceptible);
  

  for(trial = 0; trial < ntrial; ++trial){
    Rprintf("Running Trial %d\n",trial);
    sprintf(ifn,"output/%s.i.0.%d.csv",filename,trial);
    sprintf(tfn,"output/%s.t.0.%d.csv",filename,trial);
    sprintf(fn,"output/%s.%s.%s.%d.csv",filename,reduceBeta_name,eliminateSusceptibles_name,trial);
    Rprintf("init:");
    for(var = 0; var < nvar; ++var){
      Rprintf(" %d",init[var]);
    }
    Rprintf("\nnvar: %d\nntime %d\ntfn: %s\nifn: %s\nfn: %s\n",nvar,ntime,tfn,ifn,fn);
    constructTimeSeries(
      init,
      nvar,
      ntime,
      intervention_reduceBeta,
      intervention_eliminateSusceptibles,
      tfn,
      ifn,
      fn
    );
  }
  
  //Cleanup starts here

  PutRNGstate();
  return(R_NilValue);
}

/*
SEXP runCounterfactualAnalysis(
  SEXP filename,
  SEXP RreduceTransmision,
  SEXP ReliminateSusceptibles,
){
}
*/

void R2cmat(SEXP Rmat, double* *cmat, int *n, int *m){
  SEXP Rdim;
  PROTECT(Rdim = getAttrib(Rmat, R_DimSymbol));
  (*m) = INTEGER(Rdim)[0];
  (*n) = INTEGER(Rdim)[1];
  Rprintf("\t%dx%d\n",(*m),(*n));
  (*cmat) = REAL(Rmat);
  UNPROTECT(1);
  return;
}

void R2cvecdouble(SEXP Rvec, double* *cvec, int *n){
  (*n) = LENGTH(Rvec);
  (*cvec) = REAL(Rvec);
  return;
}
void R2cvecint(SEXP Rvec, int* *cvec, int *n){
  (*n) = LENGTH(Rvec);
  (*cvec) = INTEGER(AS_INTEGER(Rvec));
  return;
}
double R2cdouble(SEXP Rvec){
  if(LENGTH(Rvec) == 1){
    return((double) REAL(Rvec)[0]);
  }
  REprintf("R2cdouble only works on R vectors of length 1\n");
  return(0);
}

int R2cint(SEXP Rvec){
  Rprintf("Beginning\n");
  if(LENGTH(Rvec) == 1){
    Rprintf("Success\n");
    fflush(stdout);
    return((int) REAL(Rvec)[0]);
  }
  REprintf("R2cint only works on R vectors of length 1\n");
  Rprintf("Failed\n");
  fflush(stdout);
  return(0);
}

void R2cstring(SEXP Rstring, char* *cstring){
  (*cstring) = CHARACTER_VALUE(Rstring);
}

void c2Rdataframe(double* cframe,int nrow,int ncol ,SEXP *Rframe){
  //Note, need to install matrix somehow
  SEXP basePackage;
  SEXP Rvec;
  SEXP Rnrow;
  SEXP Rncol;
  PROTECT(
    basePackage = eval( lang2( install("getNamespace"),
      ScalarString(mkChar("base")) ),
      R_GlobalEnv
    )
  );
  PROTECT(Rnrow = NEW_INTEGER(1));
  PROTECT(Rncol = NEW_INTEGER(1));
  PROTECT(Rvec = NEW_NUMERIC(nrow*ncol));

  PROTECT(
    (*Rframe) = eval(
      lang4(
        install("matrix"),
        Rvec,
        Rnrow,
        Rncol
      ),
      R_GlobalEnv
    )
  );
  UNPROTECT(4);
  return;
}

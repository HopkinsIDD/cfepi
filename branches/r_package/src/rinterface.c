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
  ntime = R2cint(Rntime);
  ntrial = R2cint(Rntrial);
  R2cmat(Rtransitions,&transitions,&nvar1,&nvar2);
  if(nvar1 != nvar){
    Rf_error("Dimension mismatch.  The transition matrix should have one row for each variable.\n");
    return(R_NilValue);
  }
  if(nvar2 != nvar){
    Rf_error("Dimension mismatch.  The transition matrix should have one col for each variable.\n");
    return(R_NilValue);
  }
  R2cmat(Rinteractions,&interactions,&nvar1,&nvar2);
  if(nvar1 != nvar){
    Rf_error("Dimension mismatch.  The interaction matrix should have one row for each variable.\n");
    return(R_NilValue);
  }
  if(nvar2 != nvar){
    Rf_error("Dimension mismatch.  The interaction matrix should have one col for each variable.\n");
    return(R_NilValue);
  }

  //Things are loaded now
  if(CONSTRUCT_DEBUG == 1){
    Rprintf("Load successful\n");
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
  }

  for(trial = 0; trial < ntrial; ++trial){
    if(CONSTRUCT_DEBUG == 1){
      Rprintf("Running Trial %d\n",trial);
    }
    sprintf(ifn,"%s.i.0.%d.dat",filename,trial);
    sprintf(tfn,"%s.t.0.%d.dat",filename,trial);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
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
  if(RUN_DEBUG==1){
    Rprintf("Output stub is %s\n",filename);
  }
  R2cvecint(RinitialConditions,&init,&nvar);
  ntime = R2cint(Rntime);
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
    /*
    Rf_error("Fifth Call\n");
    Rprintf("length of pars is %d\n",LENGTH(RbetaPars));
    Rprintf("length of pars[[1]] is %d\n",LENGTH(VECTOR_ELT(RbetaPars,0)));
    Rprintf("pars[[1]] is %d\n",R2cint(VECTOR_ELT(RbetaPars,0)));
    Rprintf("length of pars[[2]] is %d\n",LENGTH(VECTOR_ELT(RbetaPars,0)));
    Rprintf("pars[[2]] is %f\n",R2cdouble(VECTOR_ELT(RbetaPars,1)));
    */
    param_beta = param_flat_beta(
      R2cint(VECTOR_ELT(RbetaPars,0)),
      R2cdouble(VECTOR_ELT(RbetaPars,1))
    );
  } else {
    Rf_error("Could not recognize the beta specification %s\n",reduceBeta_name);
    return(R_NilValue);
  }
  if(strcmp(eliminateSusceptibles_name,"None")==0){
    intervention_unparametrized_eliminateSusceptibles = &no_susceptible;
    param_susceptible = param_no_susceptible();
  } else if (strcmp(eliminateSusceptibles_name,"Single")==0){
    Rf_error( "This code is not yet written\n");
    return(R_NilValue);
    intervention_unparametrized_eliminateSusceptibles = &flat_susceptible;
    param_susceptible = param_no_susceptible();
  } else {
    Rf_error("Could not recognize the susceptibles specification %s\n",eliminateSusceptibles_name);
    return(R_NilValue);
  }

  //Set the parameters (This should eventually be based on the R input)
  intervention_reduceBeta = partially_evaluate_beta(intervention_unparametrized_reduceBeta,param_beta);
  intervention_eliminateSusceptibles = partially_evaluate_susceptible(intervention_unparametrized_eliminateSusceptibles,param_susceptible);
  
  sprintf(ifn,"%s.i.0.%d.dat",filename,trial);
  sprintf(tfn,"%s.t.0.%d.dat",filename,trial);
  sprintf(fn,"%s.%s.%s.%d.csv",filename,reduceBeta_name,eliminateSusceptibles_name,trial);

  if(RUN_DEBUG == 1){
    Rprintf("init:");
    for(var = 0; var < nvar; ++var){
      Rprintf(" %d",init[var]);
    }
    Rprintf("\nnvar: %d\nntime %d\ntfn: %s\nifn: %s\nfn: %s\n",nvar,ntime,tfn,ifn,fn);
  }
  for(trial = 0; trial < ntrial; ++trial){
    if(RUN_DEBUG == 1){
      Rprintf("Running Trial %d\n",trial);
    }
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
  (*n) = INTEGER(Rdim)[0];
  (*m) = INTEGER(Rdim)[1];
  // Rprintf("\t%dx%d\n",(*m),(*n));
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
  Rf_error("R2cdouble only works on R vectors of length 1\n");
  return(0);
}

int R2cint(SEXP Rvec){
  // Rprintf("Beginning\n");
  if(LENGTH(Rvec) == 1){
    // Rprintf("Success\n");
    return((int) REAL(Rvec)[0]);
  }
  Rf_error("R2cint only works on R vectors of length 1\n");
  // Rprintf("Failed\n");
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

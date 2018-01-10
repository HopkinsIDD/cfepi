#include <R.h>
#include <Rmath.h>
#include <Rinternals.h>
#include <Rdefines.h>
#include "rinterface.h"
#include "counterfactual.h"

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
  R2cint(Rntime,&ntime);
  R2cint(Rntrial,&ntrial);
  R2cmat(Rtransitions,&transitions,&nvar1,&nvar2);
  if(nvar1 != nvar){
    fprintf(stderr,"Dimension mismatch.  The transition matrix should have one row for each variable.\n");
    return(R_NilValue);
  }
  if(nvar2 != nvar){
    fprintf(stderr,"Dimension mismatch.  The transition matrix should have one col for each variable.\n");
    return(R_NilValue);
  }
  R2cmat(Rinteractions,&interactions,&nvar1,&nvar2);
  if(nvar1 != nvar){
    fprintf(stderr,"Dimension mismatch.  The interaction matrix should have one row for each variable.\n");
    return(R_NilValue);
  }
  if(nvar2 != nvar){
    fprintf(stderr,"Dimension mismatch.  The interaction matrix should have one col for each variable.\n");
    return(R_NilValue);
  }

  //Things are loaded now

  for(trial = 0; trial < ntrial; ++trial){
    printf("Running Trial %d\n",trial);
    sprintf(ifn,"output/%s.i.0.%d.csv",filename,trial);
    sprintf(tfn,"output/%s.t.0.%d.csv",filename,trial);
    printf("init:");
    for(var = 0; var < nvar; ++var){
      printf(" %d",init[var]);
    }
    printf("\nnvar: %d\nntime %d\ntransitions:\n",nvar,ntime);
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        printf(" %f",transitions[IND(var,var2,nvar)]);
      }
      printf("\n");
    }
    printf("\ninteractions:\n");
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        printf(" %f",interactions[IND(var,var2,nvar)]);
      }
      printf("\n");
    }
    printf("\ntfn: %s\nifn: %s\n",tfn,ifn);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
    // sprintf(ifn,"output/%s.i.1.%d.csv",trial);
    // sprintf(tfn,"output/%s.t.1.%d.csv",trial);
    // runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
  }
  
  //Cleanup starts here

  PutRNGstate();
  return(R_NilValue);
}

SEXP runIntervention(SEXP Rfilename, SEXP RinitialConditions, SEXP RreduceBeta, SEXP ReliminateSusceptibles, SEXP Rntime, SEXP Rntrial){
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
  R2cint(Rntime,&ntime);
  R2cint(Rntrial,&ntrial);
  //These may change later
  R2cstring(RreduceBeta,&reduceBeta_name);
  R2cstring(ReliminateSusceptibles,&eliminateSusceptibles_name);
 
  //Things are loaded now
  //Choose the type of intervention:
  if(strcmp(reduceBeta_name,"None")){
    intervention_unparametrized_reduceBeta = &no_interventionBeta;
    param_beta.time = 0;
  } else {
    printf("Could not recognize the beta specification %s\n",reduceBeta_name);
    return(R_NilValue);
  }
  if(strcmp(eliminateSusceptibles_name,"None")){
    intervention_unparametrized_eliminateSusceptibles = &no_interventionSusceptibles;
    param_beta.time = 0;
  } else {
    printf("Could not recognize the susceptibles specification %s\n",eliminateSusceptibles_name);
    return(R_NilValue);
  }

  //Set the parameters (This should eventually be based on the R input)
  intervention_unparametrized_eliminateSusceptibles = &interventionSusceptibles;
  intervention_reduceBeta = partially_evaluate_beta(intervention_unparametrized_reduceBeta,param_beta);
  intervention_eliminateSusceptibles = partially_evaluate_susceptible(intervention_unparametrized_eliminateSusceptibles,param_susceptible);
  

  for(trial = 0; trial < ntrial; ++trial){
    printf("Running Trial %d\n",trial);
    sprintf(ifn,"output/%s.i.0.%d.csv",filename,trial);
    sprintf(tfn,"output/%s.t.0.%d.csv",filename,trial);
    sprintf(fn,"output/%s.noint.%d.csv",filename,trial);
    printf("init:");
    for(var = 0; var < nvar; ++var){
      printf(" %d",init[var]);
    }
    printf("\nnvar: %d\nntime %d\ntfn: %s\nifn: %s\nfn: %s\n",nvar,ntime,tfn,ifn,fn);
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
  printf("\t%dx%d\n",(*m),(*n));
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
void R2cdouble(SEXP Rvec, double *val){
  if(LENGTH(Rvec) == 1){
    (*val) = REAL(Rvec)[0];
    return;
  }
  fprintf(stderr,"R2cdouble only works on R vectors of length 1\n");
  return;
}

void R2cint(SEXP Rvec, int *val){
  if(LENGTH(Rvec) == 1){
    (*val) = (int) REAL(Rvec)[0];
    return;
  }
  fprintf(stderr,"R2cdouble only works on R vectors of length 1\n");
  return;
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

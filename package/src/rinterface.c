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

  PROTECT(Rfilename);
  PROTECT(RinitialConditions);
  PROTECT(Rinteractions);
  PROTECT(Rtransitions);
  PROTECT(Rntime);
  PROTECT(Rntrial);

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
    Rf_warning("Load successful\n");
    Rf_warning("init:");
    for(var = 0; var < nvar; ++var){
      Rf_warning(" %d",init[var]);
    }
    Rf_warning("\nnvar: %d\nntime %d\ntransitions:\n",nvar,ntime);
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        Rf_warning(" %f",transitions[IND(var,var2,nvar)]);
      }
      Rf_warning("\n");
    }
    Rf_warning("\ninteractions:\n");
    for(var = 0; var < nvar; ++var){
      for(var2 = 0; var2 < nvar; ++var2){
        Rf_warning(" %f",interactions[IND(var,var2,nvar)]);
      }
      Rf_warning("\n");
    }
    Rf_warning("\ntfn: %s\nifn: %s\n",tfn,ifn);
  }

  for(trial = 0; trial < ntrial; ++trial){
    if(CONSTRUCT_DEBUG == 1){
      Rf_warning("Running Trial %d\n",trial);
    }
    sprintf(ifn,"%s.i.0.%d.dat",filename,trial);
    sprintf(tfn,"%s.t.0.%d.dat",filename,trial);
    runCounterfactualAnalysis("Fast",init,nvar,ntime,transitions,interactions,tfn,ifn);
  }
  
  //Cleanup starts here

  PutRNGstate();
  UNPROTECT(6);
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
  int i;
  person_t npop;

  // Parameters for (some) interventions:
  int* param_vector1;
  int param_vector_size1;
  var_t* param_vector2;
  var_t param_vector_size2;

  PROTECT(Rfilename);
  PROTECT(RinitialConditions);
  PROTECT(RreduceBeta);
  PROTECT(ReliminateSusceptibles);
  PROTECT(RbetaPars);
  PROTECT(RsusceptiblePars);
  PROTECT(Rntime);
  PROTECT(Rntrial);

  beta_t intervention_unparametrized_reduceBeta;
  param_beta_t param_beta;
  saved_beta_t intervention_reduceBeta;

  susceptible_t intervention_unparametrized_eliminateSusceptibles;
  param_susceptible_t param_susceptible;
  saved_susceptible_t intervention_eliminateSusceptibles;

  GetRNGstate();
  if(RUN_DEBUG==1){
    Rf_warning("runIntervention starting");
  }
  
  R2cstring(Rfilename,&filename);
  if(RUN_DEBUG==1){
    Rf_warning("Output stub is %s\n",filename);
  }
  R2cvecint(RinitialConditions,&init,&nvar);
  ntime = R2cint(Rntime);
  ntrial = R2cint(Rntrial);
  npop = 0;
  for(i=0;i<nvar;++i){
    npop = npop + init[i];
  }
  if(RUN_DEBUG == 1){
    Rf_warning("Simulating %d simulations over %d times.",ntrial,ntime);
  }
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
    param_beta = param_flat_beta(
      R2cint(VECTOR_ELT(RbetaPars,0)),
      R2cdouble(VECTOR_ELT(RbetaPars,1))
    );
    // Rf_warning("start_time is %d, rate is %f",(*( (data_beta_flat_t*) param_beta.data)).start_time,(*( (data_beta_flat_t*) param_beta.data)).rate);
  } else {
    Rf_error("Could not recognize the beta specification %s\n",reduceBeta_name);
    return(R_NilValue);
  }
  if(strcmp(eliminateSusceptibles_name,"None")==0){
    intervention_unparametrized_eliminateSusceptibles = &no_susceptible;
    param_susceptible = param_no_susceptible();
  } else if (strcmp(eliminateSusceptibles_name,"Constant")==0){
    intervention_unparametrized_eliminateSusceptibles = &constant_susceptible;
    // R2cvecint(VECTOR_ELT(RsusceptiblePars,3),&param_vector1,&param_vector_size1),
    // param_vector_size2 = param_vector_size2 *1;
    // param_vector2 = malloc(param_vector_size2 * sizeof(*param_vector2));
    param_susceptible = param_constant_susceptible(
      R2cint(VECTOR_ELT(RsusceptiblePars,0)),
      R2cdouble(VECTOR_ELT(RsusceptiblePars,1)),
      npop,
      R2cdouble(VECTOR_ELT(RsusceptiblePars,2)),
      R2cint(VECTOR_ELT(RsusceptiblePars,3)),
      R2cint(VECTOR_ELT(RsusceptiblePars,4)),
      1
    );
  } else if (strcmp(eliminateSusceptibles_name,"Single")==0){
    intervention_unparametrized_eliminateSusceptibles = &single_susceptible;
    // R2cvecint(VECTOR_ELT(RsusceptiblePars,3),&param_vector1,&param_vector_size1),
    // param_vector_size2 = param_vector_size2 *1;
    // param_vector2 = malloc(param_vector_size2 * sizeof(*param_vector2));
    param_susceptible = param_single_susceptible(
      R2cint(VECTOR_ELT(RsusceptiblePars,0)),
      R2cdouble(VECTOR_ELT(RsusceptiblePars,1)),
      R2cint(VECTOR_ELT(RsusceptiblePars,2)),
      R2cint(VECTOR_ELT(RsusceptiblePars,3)),
      1
    );
  } else {
    Rf_error("Could not recognize the susceptibles specification %s\n",eliminateSusceptibles_name);
    return(R_NilValue);
  }

  //Set the parameters (This should eventually be based on the R input)
  intervention_reduceBeta = partially_evaluate_beta(intervention_unparametrized_reduceBeta,param_beta);
  intervention_eliminateSusceptibles = partially_evaluate_susceptible(intervention_unparametrized_eliminateSusceptibles,param_susceptible);
  
  if(RUN_DEBUG == 1){
    Rf_warning("init:");
    for(var = 0; var < nvar; ++var){
      Rf_warning(" %d",init[var]);
    }
    Rf_warning("\nnvar: %d\nntime %d\ntfn: %s\nifn: %s\nfn: %s\n",nvar,ntime,tfn,ifn,fn);
  }
  for(trial = 0; trial < ntrial; ++trial){
    sprintf(ifn,"%s.i.0.%d.dat",filename,trial);
    sprintf(tfn,"%s.t.0.%d.dat",filename,trial);
    sprintf(fn,"%s.%s.%s.%d.csv",filename,reduceBeta_name,eliminateSusceptibles_name,trial);
    if(RUN_DEBUG == 1){
      Rf_warning("init:");
      for(var = 0; var < nvar; ++var){
        Rf_warning(" %d",init[var]);
      }
      Rf_warning("\nnvar: %d\nntime %d\ntfn: %s\nifn: %s\nfn: %s\n",nvar,ntime,tfn,ifn,fn);
    }

    if(RUN_DEBUG == 1){
      Rf_warning("Running Trial %d\n",trial);
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
  UNPROTECT(8);
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
  (*cmat) = malloc((*m)*(*n) * sizeof(**cmat));
  // Rf_warning("\t%dx%d\n",(*m),(*n));
  for(int i = 0; i < ((*n)*(*m)); ++i){
    (*cmat)[i] = REAL(Rmat)[i];
  }
  // (*cmat) = REAL(Rmat);
  UNPROTECT(1);
  return;
}

void R2cvecdouble(SEXP Rvec, double* *cvec, int *n){
  (*n) = LENGTH(Rvec);
  (*cvec) = malloc((*n) * sizeof(**cvec));
  for(int i = 0; i < (*n); ++i){
    (*cvec)[i] = REAL(Rvec)[i];
  }
  // (*cvec) = REAL(Rvec);
  return;
}

void R2cvecint(SEXP Rvec, int* *cvec, int *n){
  (*n) = LENGTH(Rvec);
  (*cvec) = malloc((*n) * sizeof(**cvec));
  if((*cvec)==NULL){
    Rf_error("Could not allocate vector");
  }
  for(int i = 0; i < (*n); ++i){
    (*cvec)[i] = INTEGER(AS_INTEGER(Rvec))[i];
  }
  // (*cvec) = INTEGER(AS_INTEGER(Rvec));
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
  // Rf_warning("Beginning\n");
  if(LENGTH(Rvec) == 1){
    // Rf_warning("Success\n");
    return((int) REAL(Rvec)[0]);
  }
  Rf_error("R2cint only works on R vectors of length 1\n");
  // Rf_warning("Failed\n");
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

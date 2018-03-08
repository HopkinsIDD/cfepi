#' @export
#' @name run_full_counterfactual
#' @title run_full_counterfactual
run_full_counterfactual <- function(
  filename,
  init,
  inter,
  trans,
  beta_type,
  susceptible_type,
  beta_pars,
  susceptible_pars,
  ntime,
  ntrial = 1000){
  setup_counterfactual(filename,init,inter,trans,ntime,ntrial)
  run_scenario(filename,init,beta_type,susceptible_type,beta_pars,susceptible_pars,ntime,ntrial)
  run_scenario(filename,init,"None","None",list(),list(),ntime,ntrial)
  return(read_scenario(filename))
}

#' @export
#' @name calculate_residual
#' @title calculate_residual
#' @param counterfactual_frame  The data frame returned by read_scenario
calculate_residual = function(counterfactual_frame){
  #' @importFrom tidyr spread
  reshaped_frame <- spread(
    #' @importFrom tidyr unite
    unite(
      #' @importFrom tidyr gather
      gather(
        counterfactual_frame,
        variable,
        value,
        #' @importFrom dplyr starts_with
        starts_with("V")
      ),
      scenario,
      beta_name,
      susceptible_name
    ),
    scenario,
    value,
  )
  #' @importFrom dplyr mutate_if
  mutate_if(reshaped_frame,
    !(colnames(reshaped_frame) %in% c("t","trial","variable")),
    #' @importFrom dplyr funs
    funs(as.numeric(.) - as.numeric(None_None))
  )
}

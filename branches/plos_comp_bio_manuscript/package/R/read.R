#' @export
#' @name read_scenario
#' @title read_scenario
#' @description Reads the results of a scenario into R.
#' @param filename The file stub used in setup_counterfactual and run_scenario
read_scenario <- function(filename,ntrial=NULL){
  stub = basename(filename)
  path = dirname(filename)
  all_files = list.files(path)
  #' @importFrom dplyr data_frame
  split_files = t(data.frame(strsplit(all_files,'.',fixed=TRUE)))
  rownames(split_files) = all_files
  colnames(split_files) = c('stub','beta_name','susceptible_name','trial','extension')
  split_files = split_files[split_files[,'stub'] == stub,]
  split_files = split_files[split_files[,'extension'] == 'csv',]
  if(!is.null(ntrial)){
    split_files = split_files[as.numeric(split_files[,'trial']) < ntrial,]
  }
  all_files = rownames(split_files)
  
  #' @importFrom dplyr bind_rows
  all_data = bind_rows(apply(
    split_files,
    1,
    function(fields){
      filename = paste(path,paste(fields,collapse='.'),sep='/')
      y = read.csv(filename,header=FALSE)
      t = 1:nrow(y)
      y$beta_name = fields['beta_name']
      y$susceptible_name = fields['susceptible_name']
      y$trial = fields['trial']
      y$t = t
      return(y)
    }
  ))
  return(all_data)
}

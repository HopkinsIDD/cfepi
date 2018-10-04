#' @export
#' @name read_scenario
#' @title read_scenario
#' @description Reads the results of a counterfactual scenario into R.
#' @param filename The file stub used in setup_counterfactual and run_scenario
#' @param ntrial optional parameter for number of trials to read (up to the maximum run)
#' @import methods
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
      #' @importFrom utils read.csv
      y = read.csv(filename,header=FALSE)
      t = 1:nrow(y)
      y$beta_name = fields['beta_name']
      y$susceptible_name = fields['susceptible_name']
      y$trial = fields['trial']
      y$t = t
      return(y)
    }
  ))
  all_data = unite(all_data,scenario,beta_name,susceptible_name)
  all_data = gather(all_data,variable,value,starts_with('V'))
  return(all_data)
}

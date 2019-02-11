#' @export
#' @name read_scenario
#' @title read_scenario
#' @description Reads the results of a counterfactual scenario into R.
#' @param filename The file stub used in setup_counterfactual and run_scenario.  If a
#' @param folder is provided, this will read any scenario in the folder.
#' @param ntrial optional parameter for number of trials to read (up to the maximum run)
#' @import methods
#' @examples
#' 

read_scenario <- function(scenario,intervention,ntrial,path='.'){
  if(!dir.exists(path))
    stop("path should be a directory")
  
  #' @importFrom tidyr crossing
  split_files = crossing(scenario = scenario,intervention = intervention,trial = 0:(ntrial-1),extension='csv')
  #' @importFrom dplyr mutate
  split_files = mutate(split_files,filename = paste(path,paste(scenario,intervention,trial,extension,sep='.'),sep='/'))

  files_to_check = split_files$filename[which(split_files$trial == (ntrial-1))]
  
  if(!all(file.exists(files_to_check)))
    stop(paste("Could not find the file",files_to_check[!file.exists(files_to_check)],"Ensure that you have run the appropriate scenarios and interventions."))
	
  #' @importFrom dplyr bind_rows
  all_data = bind_rows(apply(
    split_files,
    1,
    function(fields){
      #' @importFrom utils read.csv
      y = read.csv(fields[['filename']],header=FALSE)
      t = 1:nrow(y)
      y$scenario = fields['scenario']
      y$intervention = fields['intervention']
      y$trial = fields['trial']
      y$t = t
      if(nrow(y)==0){browser()}
      return(y)
    }
  ))
  #' @importFrom tidyr gather
  #' @importFrom dplyr starts_with
  all_data = gather(all_data,'variable','value',starts_with('V'))
  return(all_data)
}

try({remove.packages('cfepi')},silent=T)
install.packages('package',type='source',repos=NULL)
library(cfepi)

# source("R/setup_null_intervention.R")
# source("R/setup_antiviral_intervention.R")
source("R/setup_vaccination_intervention.R")
# source("R/setup_handwashing_intervention.R")

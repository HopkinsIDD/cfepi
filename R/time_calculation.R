library(lubridate)
setup_start = hms("10:44:00")
setup_end = hms("12:15:00")
null_start = setup_end
null_end = hms("13:15:00")
constant_start = null_end
constant_end = hms("13:47:00")
single_start = constant_end
single_end = hms("14:17:00")
flat_start = single_end  
flat_end = hms("14:45:00")

as.duration(setup_end - setup_start)
as.duration(null_end - null_start)
as.duration(constant_end - constant_start)
as.duration(single_end - single_start)
as.duration(flat_end - flat_start)

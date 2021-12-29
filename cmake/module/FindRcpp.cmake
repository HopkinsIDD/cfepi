include (FindPackageHandleStandardArgs)

find_package("R" REQUIRED)

find_program(RSCRIPT_COMMAND Rscript DOC "Rscript executable.")

if(RSCRIPT_COMMAND)
  execute_process(COMMAND ${RSCRIPT_COMMAND} "-e" "cat(.libPaths()[[1]])" OUTPUT_VARIABLE R_LIBPATHS)
  file(GLOB_RECURSE RCPP_LIBRARY_HINT "${R_LIBPATHS}/*/*/Rcpp.h")
  string(REGEX REPLACE "include/Rcpp.h$" "libs" RCPP_LIBRARY_HINT "${RCPP_LIBRARY_HINT}")
  find_library(RCPP_LIBRARY NAMES "Rcpp.so" HINTS "${RCPP_LIBRARY_HINT}" REQUIRED)

  file(GLOB_RECURSE RCPP_HINT "${R_LIBPATHS}/*/*/Rcpp.h")
  file(GLOB_RECURSE RCPP_INCLUDE_HINT "${R_LIBPATHS}/*/*/Rcpp.h")
  string(REGEX REPLACE "/Rcpp.h$" "" RCPP_INCLUDE_HINT "${RCPP_INCLUDE_HINT}")
  find_path(RCPP_INCLUDE_DIR NAMES "Rcpp.h" HINTS "${RCPP_INCLUDE_HINT}" REQUIRED)

  find_package_handle_standard_args(Rcpp HANDLE_COMPONENTS)

  if (Rcpp_FOUND)
    mark_as_advanced(RCPP_INCLUDE_DIR)
    mark_as_advanced(RCPP_LIBRARY)
  else()
    message(FATAL_ERROR "FAILURE")
  endif()

  if (Rcpp_FOUND AND NOT TARGET R::Rcpp)
    add_library(R::Rcpp IMPORTED STATIC)
    set_property(TARGET R::Rcpp PROPERTY IMPORTED_LOCATION ${RCPP_LIBRARY})
    target_include_directories(R::Rcpp INTERFACE ${RCPP_INCLUDE_DIR})
    message("RCPP_LIBRARY is ${RCPP_LIBRARY}")
    message("RCPP_INCLUDE_DIR is ${RCPP_INCLUDE_DIR}")
  endif()
else()
  message(FATAL_ERROR "Could not find Rscript. Is it installed?")
endif()

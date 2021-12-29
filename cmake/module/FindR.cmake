include (FindPackageHandleStandardArgs)

find_program(R_COMMAND R DOC "R executable.")

if(R_COMMAND)
  execute_process(
    WORKING_DIRECTORY .
    COMMAND ${R_COMMAND} RHOME
    OUTPUT_VARIABLE R_ROOT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(R_HOME ${R_ROOT_DIR} CACHE PATH "R home directory obtained from R RHOME")

  find_library(R_LIBRARY libR.so
    HINTS ${R_ROOT_DIR}
    PATHS /usr/local/lib /usr/local/lib64 /usr/share
    PATH_SUFFIXES include R/include
    DOC "Path to file libR.so")
  find_path(R_INCLUDE_DIR R.h
    HINTS ${R_ROOT_DIR}
    PATHS /usr/local/lib /usr/local/lib64 /usr/share
    PATH_SUFFIXES include R/include
    DOC "Path to file R.h")

  find_package_handle_standard_args(R HANDLE_COMPONENTS)
  if(R_FOUND)
    mark_as_advanced(R_INCLUDE_DIR)
  else()
    message(
      FATAL_ERROR
      "R.h file not found. Locations tried:\n"
      "/usr/local/lib /usr/local/lib64 /usr/share ${R_ROOT_DIR}"
      )
  endif()
  if(R_FOUND)
    add_library(R::R IMPORTED STATIC)
    set_property(TARGET R::R PROPERTY IMPORTED_LOCATION ${R_LIBRARY})
    target_include_directories(R::R INTERFACE ${R_INCLUDE_DIR})
    message("R_LIBRARY is ${R_LIBRARY}")
    message("R_INCLUDE_DIR is ${R_INCLUDE_DIR}")
  endif()
endif()

# Generic test that uses conan libs
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/sir.json JSON_CONFIG_CONTENTS_RAW)
STRING(REPLACE ";" "" JSON_CONFIG_CONTENTS ${JSON_CONFIG_CONTENTS_RAW})
message(${JSON_CONFIG_CONTENTS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp ESCAPE_QUOTES NEWLINE_STYLE UNIX)
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/uvira.json JSON_CONFIG_CONTENTS_RAW)
STRING(REPLACE ";" "" JSON_CONFIG_CONTENTS ${JSON_CONFIG_CONTENTS_RAW})
message(${JSON_CONFIG_CONTENTS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/uvira.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/uvira.cpp ESCAPE_QUOTES NEWLINE_STYLE UNIX)

find_package(range-v3 CONFIG)
find_package(rangesnext CONFIG)
find_package(daw-json-link CONFIG REQUIRED)
find_package(coz-profiler CONFIG)

add_library(cfepi INTERFACE)
target_include_directories(cfepi INTERFACE include)
target_link_libraries(
  cfepi
  INTERFACE
          project_options
          project_warnings
          range-v3::range-v3
          rangesnext::rangesnext
          daw::daw-json-link
          coz::coz
)
add_executable(cfepi_test tests.cpp)
target_link_libraries(cfepi_test cfepi)
add_executable(uvira uvira.cpp)
target_link_libraries(uvira cfepi)

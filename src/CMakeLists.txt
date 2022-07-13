# Generic test that uses conan libs
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/sir.json TEST_JSON_CONFIG_CONTENTS_RAW)
STRING(REPLACE ";" "" TEST_JSON_CONFIG_CONTENTS ${TEST_JSON_CONFIG_CONTENTS_RAW})
message(${TEST_JSON_CONFIG_CONTENTS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp ESCAPE_QUOTES NEWLINE_STYLE UNIX)
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/sir.json SIR_JSON_CONFIG_CONTENTS_RAW)
STRING(REPLACE ";" "" SIR_JSON_CONFIG_CONTENTS ${SIR_JSON_CONFIG_CONTENTS_RAW})
message(${SIR_JSON_CONFIG_CONTENTS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sir.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/sir.cpp ESCAPE_QUOTES NEWLINE_STYLE UNIX)
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/uvira.json UVIRA_JSON_CONFIG_CONTENTS_RAW)
STRING(REPLACE ";" "" UVIRA_JSON_CONFIG_CONTENTS ${UVIRA_JSON_CONFIG_CONTENTS_RAW})
message(${UVIRA_JSON_CONFIG_CONTENTS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/uvira.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/uvira.cpp ESCAPE_QUOTES NEWLINE_STYLE UNIX)
FILE(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/covid.json COVID_JSON_CONFIG_CONTENTS_RAW)
STRING(REPLACE ";" "" COVID_JSON_CONFIG_CONTENTS ${COVID_JSON_CONFIG_CONTENTS_RAW})
message(${COVID_JSON_CONFIG_CONTENTS})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/covid.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/covid.cpp ESCAPE_QUOTES NEWLINE_STYLE UNIX)

find_package(range-v3 CONFIG)
find_package(rangesnext CONFIG)
find_package(daw-json-link CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(cfepi INTERFACE)
target_include_directories(cfepi INTERFACE include)
target_include_directories(cfepi INTERFACE optick)
target_link_libraries(
  cfepi
  INTERFACE
          project_options
          project_warnings
          range-v3::range-v3
          rangesnext::rangesnext
          daw::daw-json-link
          fmt::fmt
)
add_executable(cfepi_test tests.cpp)
target_link_libraries(cfepi_test cfepi)
add_executable(uvira uvira.cpp)
target_link_libraries(uvira cfepi)
add_executable(covid covid.cpp)
target_link_libraries(covid cfepi)
add_executable(sir sir.cpp)
target_link_libraries(sir cfepi)

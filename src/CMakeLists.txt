# Generic test that uses conan libs

add_library(cfepi INTERFACE)
target_include_directories(cfepi INTERFACE include)
target_link_libraries(
  cfepi
  INTERFACE project_options
          project_warnings
          CONAN_PKG::threadpool
          CONAN_PKG::range-v3
          CONAN_PKG::rangesnext
)

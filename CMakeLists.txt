cmake_minimum_required(VERSION 3.30)
project(corncob VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(FetchContent)

option(CORNCOB_STATIC_RUNTIME "Link compiler runtimes statically" ${MINGW})
if(CORNCOB_STATIC_RUNTIME)
  if(MINGW)
    add_link_options(-static)
    set(Boost_USE_STATIC_LIBS ON)
  else()
    add_link_options("$<$<LINK_LANG_AND_ID:CXX,GNU>:-static-libgcc;-static-libstdc++>")
  endif()
endif()

find_package(Boost 1.85 CONFIG REQUIRED COMPONENTS program_options)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build GLFW documentation")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build GLFW examples")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build GLFW tests")
set(GLFW_INSTALL OFF CACHE BOOL "Install GLFW")
FetchContent_Declare(glfw
  URL https://codeload.github.com/glfw/glfw/tar.gz/a74efa0d5628b74adc0426af4c5710e287fa7c2c
  URL_HASH SHA256=7e85e39ade1efdda00734d323a0230b5c938074e9fb2ddbe453bdbb281064d2c
)
FetchContent_MakeAvailable(glfw)

function(configure_first_party target)
  target_compile_features(${target} PUBLIC cxx_std_26)
  target_include_directories(${target} PUBLIC "${PROJECT_SOURCE_DIR}/src")
  set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic;-Wconversion>
    $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-UNDEBUG>
    $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:MSVC>>:/UNDEBUG>
  )
endfunction()

# Embed original compressed resource bytes using CMake alone; no runtime file access or Python build dependency
set(cockpit_asset "${PROJECT_SOURCE_DIR}/assets/cockpit.img")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${cockpit_asset}")
file(READ "${cockpit_asset}" cockpit_hex HEX)
file(SIZE "${cockpit_asset}" cockpit_size)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " cockpit_bytes "${cockpit_hex}")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated")
configure_file(cmake/embedded_assets.cpp.in generated/embedded_assets.cpp @ONLY)

add_library(corncob_core STATIC
  src/render/framebuffer.cpp
  src/render/framebuffer.h
  src/resources/assets.h
  src/resources/cockpit_mask.cpp
  src/resources/cockpit_mask.h
  "${PROJECT_BINARY_DIR}/generated/embedded_assets.cpp"
)
configure_first_party(corncob_core)

add_library(corncob_platform STATIC
  src/platform/framebuffer_presenter.cpp
  src/platform/framebuffer_presenter.h
)
target_link_libraries(corncob_platform
  PUBLIC corncob_core
  PRIVATE glfw
  PRIVATE OpenGL::GL
)
configure_first_party(corncob_platform)

add_executable(corncob src/main.cpp)
target_link_libraries(corncob
  PRIVATE corncob_platform
  PRIVATE glfw
  PRIVATE Boost::headers
  PRIVATE Boost::program_options
)
configure_first_party(corncob)

if(BUILD_TESTING)
  add_executable(resource_checks tests/resource_checks.cpp)
  target_link_libraries(resource_checks PRIVATE corncob_core)
  configure_first_party(resource_checks)
  add_test(NAME embedded_cockpit COMMAND resource_checks)
endif()

install(TARGETS corncob RUNTIME DESTINATION .)

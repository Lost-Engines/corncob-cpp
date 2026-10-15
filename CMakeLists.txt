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

# Embed original resources with CMake alone; the application performs no asset file I/O
function(embed_resource resource_name asset_file)
  set(resource_path "${PROJECT_SOURCE_DIR}/assets/${asset_file}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${resource_path}")
  file(READ "${resource_path}" resource_hex HEX)
  file(SIZE "${resource_path}" resource_size)
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " resource_bytes "${resource_hex}")
  configure_file(cmake/embedded_assets.cpp.in "generated/${resource_name}.cpp" @ONLY)
  target_sources(corncob_core PRIVATE "${PROJECT_BINARY_DIR}/generated/${resource_name}.cpp")
endfunction()

add_library(corncob_core STATIC
  src/graphics/raster.cpp
  src/graphics/raster.h
  src/graphics/scene.cpp
  src/graphics/scene.h
  src/maths/fixed.cpp
  src/maths/fixed.h
  src/render/framebuffer.cpp
  src/render/framebuffer.h
  src/resources/assets.h
  src/resources/geometry_bank.cpp
  src/resources/geometry_bank.h
  src/world/world.cpp
  src/world/world.h
  src/resources/cockpit_mask.cpp
  src/resources/cockpit_mask.h
)
configure_first_party(corncob_core)
target_include_directories(corncob_core SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}")
target_compile_definitions(corncob_core PUBLIC VECTORSTORM_NO_BOOST)
embed_resource(cockpit cockpit.img)
embed_resource(engine_data engine.dat)
embed_resource(white_sands white_sands.cct)
embed_resource(morning morning.cct)
embed_resource(dusk dusk.cct)
embed_resource(wasteland wasteland.cct)

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
  add_test(NAME embedded_resources COMMAND resource_checks)
  add_executable(fixed_math_checks tests/fixed_math_checks.cpp)
  target_link_libraries(fixed_math_checks PRIVATE corncob_core)
  configure_first_party(fixed_math_checks)
  add_test(NAME native_fixed_math COMMAND fixed_math_checks "${PROJECT_SOURCE_DIR}/tests/reference/fixed_math.bin")
  add_executable(raster_checks tests/raster_checks.cpp)
  target_link_libraries(raster_checks PRIVATE corncob_core)
  configure_first_party(raster_checks)
  add_test(NAME native_raster COMMAND raster_checks)
  add_executable(scene_checks tests/scene_checks.cpp)
  target_link_libraries(scene_checks PRIVATE corncob_core)
  configure_first_party(scene_checks)
  add_test(NAME original_theatres COMMAND scene_checks)
endif()

install(TARGETS corncob RUNTIME DESTINATION .)

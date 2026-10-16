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
  src/game/actors.cpp
  src/game/actors.h
  src/game/combat.cpp
  src/game/combat.h
  src/game/front_end.cpp
  src/game/front_end.h
  src/game/campaign.cpp
  src/game/campaign.h
  src/platform/storage.cpp
  src/platform/storage.h
  src/game/pilots.cpp
  src/game/pilots.h
  src/game/statistics.cpp
  src/game/statistics.h
  src/game/session.cpp
  src/game/session.h
  src/flight/aircraft.cpp
  src/flight/aircraft.h
  src/flight/damage.cpp
  src/flight/damage.h
  src/flight/controls.cpp
  src/flight/controls.h
  src/flight/landing.cpp
  src/flight/landing.h
  src/graphics/instruments.cpp
  src/graphics/instruments.h
  src/graphics/raster.cpp
  src/graphics/raster.h
  src/graphics/scene.cpp
  src/graphics/scene.h
  src/maths/fixed.cpp
  src/maths/fixed.h
  src/render/framebuffer.cpp
  src/render/framebuffer.h
  src/resources/assets.h
  src/resources/collection.cpp
  src/resources/collection.h
  src/graphics/interface.cpp
  src/graphics/interface.h
  src/resources/geometry_bank.cpp
  src/resources/geometry_bank.h
  src/world/motion.cpp
  src/world/motion.h
  src/world/world.cpp
  src/world/world.h
  src/resources/cockpit_mask.cpp
  src/resources/cockpit_mask.h
)
configure_first_party(corncob_core)
target_include_directories(corncob_core SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}")
target_compile_definitions(corncob_core PUBLIC VECTORSTORM_NO_BOOST)
embed_resource(front_end front_end.dat)
embed_resource(cockpit cockpit.img)
embed_resource(engine_data engine.dat)
embed_resource(white_sands white_sands.cct)
embed_resource(morning morning.cct)
embed_resource(dusk dusk.cct)
embed_resource(wasteland wasteland.cct)
embed_resource(white_sands_briefing white_sands.twr)
embed_resource(morning_briefing morning.twr)
embed_resource(dusk_briefing dusk.twr)
embed_resource(wasteland_briefing wasteland.twr)


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
  add_executable(flight_checks tests/flight_checks.cpp)
  target_link_libraries(flight_checks PRIVATE corncob_core)
  configure_first_party(flight_checks)
  add_test(NAME native_flight COMMAND flight_checks "${PROJECT_SOURCE_DIR}/tests/reference/flight.bin")
  add_executable(control_checks tests/control_checks.cpp)
  target_link_libraries(control_checks PRIVATE corncob_core)
  configure_first_party(control_checks)
  add_test(NAME native_controls COMMAND control_checks "${PROJECT_SOURCE_DIR}/tests/reference/controls.bin")
  add_executable(landing_checks tests/landing_checks.cpp)
  target_link_libraries(landing_checks PRIVATE corncob_core)
  configure_first_party(landing_checks)
  add_test(NAME native_landing COMMAND landing_checks "${PROJECT_SOURCE_DIR}/tests/reference/landing.bin")
  add_executable(session_checks tests/session_checks.cpp)
  target_link_libraries(session_checks PRIVATE corncob_core)
  configure_first_party(session_checks)
  add_test(NAME scripted_session COMMAND session_checks)
endif()

install(TARGETS corncob RUNTIME DESTINATION .)

if(BUILD_TESTING)
  add_executable(motion_checks tests/motion_checks.cpp)
  configure_first_party(motion_checks)
  target_include_directories(motion_checks PRIVATE "${PROJECT_SOURCE_DIR}")
  target_link_libraries(motion_checks PRIVATE corncob_core)
  add_test(NAME native_motion COMMAND motion_checks "${PROJECT_SOURCE_DIR}/tests/reference/motion.bin")
endif()

if(BUILD_TESTING)
  add_executable(collision_checks tests/collision_checks.cpp)
  configure_first_party(collision_checks)
  target_include_directories(collision_checks PRIVATE "${PROJECT_SOURCE_DIR}")
  target_link_libraries(collision_checks PRIVATE corncob_core)
  add_test(NAME native_collisions COMMAND collision_checks "${PROJECT_SOURCE_DIR}/tests/reference/collision.bin")
endif()

if(BUILD_TESTING)
  add_executable(combat_checks tests/combat_checks.cpp)
  configure_first_party(combat_checks)
  target_link_libraries(combat_checks PRIVATE corncob_core)
  add_test(NAME scripted_combat COMMAND combat_checks)
endif()

if(BUILD_TESTING)
  add_executable(damage_checks tests/damage_checks.cpp)
  configure_first_party(damage_checks)
  target_include_directories(damage_checks PRIVATE "${PROJECT_SOURCE_DIR}")
  target_link_libraries(damage_checks PRIVATE corncob_core)
  add_test(NAME native_damage COMMAND damage_checks "${PROJECT_SOURCE_DIR}/tests/reference/damage.bin")
endif()

if(BUILD_TESTING)
  add_executable(actor_checks tests/actor_checks.cpp)
  configure_first_party(actor_checks)
  target_include_directories(actor_checks PRIVATE "${PROJECT_SOURCE_DIR}")
  target_link_libraries(actor_checks PRIVATE corncob_core)
  add_test(NAME native_actors COMMAND actor_checks "${PROJECT_SOURCE_DIR}/tests/reference/actors.bin")
endif()

if(BUILD_TESTING)
  add_executable(scenario_checks tests/scenario_checks.cpp)
  configure_first_party(scenario_checks)
  target_link_libraries(scenario_checks PRIVATE corncob_core)
  add_test(NAME scripted_scenarios COMMAND scenario_checks)
endif()

if(BUILD_TESTING)
  add_executable(interface_checks tests/interface_checks.cpp)
  configure_first_party(interface_checks)
  target_link_libraries(interface_checks PRIVATE corncob_core)
  add_test(NAME native_interface COMMAND interface_checks)
endif()

if(BUILD_TESTING)
  add_executable(score_checks tests/score_checks.cpp)
  configure_first_party(score_checks)
  target_include_directories(score_checks PRIVATE "${PROJECT_SOURCE_DIR}")
  target_link_libraries(score_checks PRIVATE corncob_core)
  add_test(NAME native_score COMMAND score_checks "${PROJECT_SOURCE_DIR}/tests/reference/score.bin")
endif()

if(BUILD_TESTING)
  add_executable(pilot_checks tests/pilot_checks.cpp)
  configure_first_party(pilot_checks)
  target_include_directories(pilot_checks PRIVATE "${PROJECT_SOURCE_DIR}")
  target_link_libraries(pilot_checks PRIVATE corncob_core)
  add_test(NAME native_pilots COMMAND pilot_checks "${PROJECT_SOURCE_DIR}/tests/reference/pilots.bin")
endif()

if(BUILD_TESTING)
  add_executable(campaign_checks tests/campaign_checks.cpp)
  configure_first_party(campaign_checks)
  target_link_libraries(campaign_checks PRIVATE corncob_core)
  add_test(NAME persistent_campaign COMMAND campaign_checks "${PROJECT_BINARY_DIR}/campaign-checks")
endif()

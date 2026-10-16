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

FetchContent_Declare(miniaudio
  URL https://codeload.github.com/mackron/miniaudio/tar.gz/f40cf03f80cdb7e741d43e53b7e706e8c1394bcf
  URL_HASH SHA256=412326cf55133404cbfb81ec8974b10149dc68732ce5bdeee8ba9cfc2695d646
  SOURCE_SUBDIR unused_upstream_build
)
FetchContent_Declare(nuked_opl3
  URL https://codeload.github.com/nukeykt/Nuked-OPL3/tar.gz/765ec962e473aeb767e4cba74ffdc8f588ffbfe8
  URL_HASH SHA256=2fad908c3904d3ef51b55e0d6a8980c7142942e2d0baa7e99bf038cf0a41199d
  SOURCE_SUBDIR unused_upstream_build
)
FetchContent_MakeAvailable(miniaudio nuked_opl3)
add_library(nuked_opl3 STATIC "${nuked_opl3_SOURCE_DIR}/opl3.c")
target_include_directories(nuked_opl3 SYSTEM PUBLIC "${nuked_opl3_SOURCE_DIR}")
target_compile_options(nuked_opl3 PRIVATE $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-O2>)
add_library(corncob_pcm STATIC src/platform/miniaudio_implementation.c)
target_include_directories(corncob_pcm SYSTEM PUBLIC "${miniaudio_SOURCE_DIR}")
target_compile_definitions(corncob_pcm PRIVATE MA_NO_DECODING MA_NO_ENCODING MA_NO_RESOURCE_MANAGER MA_NO_NODE_GRAPH MA_NO_ENGINE)
target_compile_options(corncob_pcm PRIVATE $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-O2>)
find_package(Threads REQUIRED)
target_link_libraries(corncob_pcm PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(APPLE)
  target_link_libraries(corncob_pcm PRIVATE "-framework CoreAudio" "-framework AudioToolbox" "-framework CoreFoundation")
elseif(WIN32)
  target_link_libraries(corncob_pcm PRIVATE ole32)
endif()

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
  src/game/joystick.cpp
  src/game/joystick.h
  src/game/vision.cpp
  src/game/vision.h
  src/game/combat.cpp
  src/game/combat.h
  src/game/front_end.cpp
  src/game/front_end.h
  src/game/campaign.cpp
  src/game/campaign.h
  src/platform/storage.cpp
  src/platform/storage.h
  src/game/high_scores.cpp
  src/game/high_scores.h
  src/game/pilots.cpp
  src/game/pilots.h
  src/game/statistics.cpp
  src/game/statistics.h
  src/game/session.cpp
  src/game/session.h
  src/game/map_view.cpp
  src/game/map_view.h
  src/game/pilot_motion.cpp
  src/game/pilot_motion.h
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
  src/graphics/homing.cpp
  src/graphics/homing.h
  src/graphics/raster.cpp
  src/graphics/raster.h
  src/graphics/radar.cpp
  src/graphics/radar.h
  src/graphics/map_projection.cpp
  src/graphics/map_projection.h
  src/graphics/scene.cpp
  src/graphics/scene.h
  src/graphics/space_sequence.cpp
  src/graphics/space_sequence.h
  src/maths/fixed.cpp
  src/maths/fixed.h
  src/render/framebuffer.cpp
  src/render/framebuffer.h
  src/render/palette.cpp
  src/render/palette.h
  src/resources/assets.h
  src/resources/collection.cpp
  src/resources/collection.h
  src/resources/radio_messages.cpp
  src/resources/radio_messages.h
  src/graphics/interface.cpp
  src/graphics/interface.h
  src/resources/geometry_bank.cpp
  src/resources/geometry_bank.h
  src/resources/model_animation.cpp
  src/resources/model_animation.h
  src/world/motion.cpp
  src/world/motion.h
  src/world/training.cpp
  src/world/training.h
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


add_library(corncob_audio STATIC src/audio/effects.cpp src/audio/effects.h)
configure_first_party(corncob_audio)
target_link_libraries(corncob_audio PUBLIC corncob_core PRIVATE nuked_opl3)

add_library(corncob_platform STATIC
  src/platform/joystick.cpp
  src/platform/joystick.h
  src/platform/audio_output.cpp
  src/platform/audio_output.h
  src/platform/framebuffer_presenter.cpp
  src/platform/framebuffer_presenter.h
)
target_link_libraries(corncob_platform
  PUBLIC corncob_core
  PRIVATE glfw
  PRIVATE OpenGL::GL
  PRIVATE corncob_pcm
)
configure_first_party(corncob_platform)

add_executable(corncob src/main.cpp)
target_link_libraries(corncob
  PRIVATE corncob_platform
  PRIVATE corncob_audio
  PRIVATE glfw
  PRIVATE Boost::headers
  PRIVATE Boost::program_options
)
configure_first_party(corncob)

install(TARGETS corncob RUNTIME DESTINATION .)

if(BUILD_TESTING)
  function(corncob_check target test_name)
    add_executable(${target} "tests/${target}.cpp")
    configure_first_party(${target})
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}")
    target_link_libraries(${target} PRIVATE corncob_core)
    add_test(NAME ${test_name} COMMAND ${target} ${ARGN})
  endfunction()

  corncob_check(resource_checks embedded_resources "${PROJECT_SOURCE_DIR}/tests/reference/legacy_world.bin")
  corncob_check(fixed_math_checks native_fixed_math "${PROJECT_SOURCE_DIR}/tests/reference/fixed_math.bin")
  corncob_check(raster_checks native_raster)
  corncob_check(scene_checks original_theatres)
  corncob_check(flight_checks native_flight "${PROJECT_SOURCE_DIR}/tests/reference/flight.bin")
  corncob_check(control_checks native_controls "${PROJECT_SOURCE_DIR}/tests/reference/controls.bin")
  corncob_check(landing_checks native_landing "${PROJECT_SOURCE_DIR}/tests/reference/landing.bin" "${PROJECT_SOURCE_DIR}/tests/reference/landing_recovery.bin")
  corncob_check(session_checks scripted_session)
  corncob_check(motion_checks native_motion "${PROJECT_SOURCE_DIR}/tests/reference/motion.bin")
  corncob_check(collision_checks native_collisions "${PROJECT_SOURCE_DIR}/tests/reference/collision.bin")
  corncob_check(combat_checks scripted_combat)
  corncob_check(damage_checks native_damage "${PROJECT_SOURCE_DIR}/tests/reference/damage.bin")
  corncob_check(actor_checks native_actors "${PROJECT_SOURCE_DIR}/tests/reference/actors.bin")
  corncob_check(scenario_checks scripted_scenarios)
  corncob_check(interface_checks native_interface)
  corncob_check(score_checks native_score "${PROJECT_SOURCE_DIR}/tests/reference/score.bin")
  corncob_check(pilot_checks native_pilots "${PROJECT_SOURCE_DIR}/tests/reference/pilots.bin")
  corncob_check(campaign_checks persistent_campaign "${PROJECT_BINARY_DIR}/campaign-checks")
  corncob_check(training_checks native_training "${PROJECT_SOURCE_DIR}/tests/reference/training.bin")
  corncob_check(debris_checks native_debris "${PROJECT_SOURCE_DIR}/tests/reference/debris.bin")
  corncob_check(autopilot_checks native_autopilot "${PROJECT_SOURCE_DIR}/tests/reference/autopilot.bin")
  corncob_check(audio_checks native_sound "${PROJECT_SOURCE_DIR}/tests/reference/audio.bin")
  corncob_check(high_score_checks native_high_scores)
  corncob_check(space_checks native_ending "${PROJECT_SOURCE_DIR}/tests/reference/space.bin")
  corncob_check(model_animation_checks native_model_animation "${PROJECT_SOURCE_DIR}/tests/reference/model_animation.bin")
  corncob_check(radar_checks native_radar "${PROJECT_SOURCE_DIR}/tests/reference/radar.bin")
  corncob_check(map_checks native_map "${PROJECT_SOURCE_DIR}/tests/reference/map.bin")
  corncob_check(person_checks native_person "${PROJECT_SOURCE_DIR}/tests/reference/person.bin")
  corncob_check(saucer_checks native_saucers "${PROJECT_SOURCE_DIR}/tests/reference/saucers.bin")
  corncob_check(threat_checks native_threats "${PROJECT_SOURCE_DIR}/tests/reference/threats.bin")
  corncob_check(endurance_checks sustained_gameplay)
  corncob_check(joystick_checks native_joystick "${PROJECT_SOURCE_DIR}/tests/reference/joystick.bin")
  corncob_check(vehicle_checks native_vehicles "${PROJECT_SOURCE_DIR}/tests/reference/vehicles.bin")
  corncob_check(vision_checks native_visual_feedback "${PROJECT_SOURCE_DIR}/tests/reference/vision.bin")
  target_link_libraries(audio_checks PRIVATE corncob_audio)
endif()

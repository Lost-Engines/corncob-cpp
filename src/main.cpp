#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include "game/session.h"
#include "game/front_end.h"
#include <boost/program_options.hpp>
#include <boost/scope/scope_exit.hpp>
#include <GLFW/glfw3.h>
#include "graphics/scene.h"
#include "platform/framebuffer_presenter.h"
#include "platform/storage.h"
#include "platform/audio_output.h"
#include "audio/effects.h"
#include "render/framebuffer.h"
#include "resources/assets.h"
#include "resources/cockpit_mask.h"
#include "resources/geometry_bank.h"
#include "world/world.h"

namespace {

struct window_state {
  std::unique_ptr<corncob::game::session> &game;
  corncob::game::front_end &front;
  bool &sound_enabled;
};

corncob::world::theatre load_theatre(std::string const &name) {
  /// Re-entering the native front end selects embedded original scenery and its matching intelligence
  std::span<uint8_t const> world, briefing;
  if(name == "morning") {
    world = corncob::resources::morning_asset();
    briefing = corncob::resources::morning_briefing_asset();
  } else if(name == "dusk") {
    world = corncob::resources::dusk_asset();
    briefing = corncob::resources::dusk_briefing_asset();
  } else if(name == "wasteland") {
    world = corncob::resources::wasteland_asset();
    briefing = corncob::resources::wasteland_briefing_asset();
  } else if(name == "white_sands") {
    world = corncob::resources::white_sands_asset();
    briefing = corncob::resources::white_sands_briefing_asset();
  } else throw std::invalid_argument{"--theatre must be white_sands, morning, dusk or wasteland"};
  auto result{corncob::world::decode_theatre(world)};
  corncob::world::apply_briefing(result, briefing);
  return result;
}

int startup_failure(std::string const &message) {
  /// Report actionable startup errors while leaving unexpected engine failures uncaught
  std::cerr << "ERROR: " << message << std::endl;
  return EXIT_FAILURE;
}

} // anonymous namespace

auto main(int const argc, char const *const argv[])->int {
  /// Render the embedded original theatre while bringing up the native flight engine
  boost::program_options::options_description options{"Corncob 3D: reconstruction in progress"};
  options.add_options()
    ("help,h", "show command-line options")
    ("version", "show the native reconstruction version")
    ("no-sound", "start with sound effects disabled; S toggles effects during flight")
    ("skip-intro", "open the main menu without the title and story")
    ("theatre", boost::program_options::value<std::string>()->default_value("white_sands"), "original theatre: white_sands, morning, dusk or wasteland")
    ("save-dir", boost::program_options::value<std::string>()->default_value(corncob::platform::default_save_directory().string()), "directory for pilot careers and persistent theatres")
    ("scale", boost::program_options::value<int>(), "initial window scale: positive multiple of 640 x 480 (default: fit available screen)");
#ifndef NDEBUG
  options.add_options()
    ("free-camera", "developer use: inspect scenery with WASD, arrow keys and Page Up/Down; Shift moves faster, Home resets")
    ("no-cockpit", "developer use: show the scene using the original full-screen viewport")
    ("cockpit-preview", "developer use: display the raw cockpit resource instead of scenery")
    ("screenshot", boost::program_options::value<std::string>(), "developer use: save the initial 640 x 350 scene as PPM and exit without a window")
    ("seconds", boost::program_options::value<double>()->default_value(0.0), "developer use: close after this many seconds; zero disables the time limit");
#endif // NDEBUG
  boost::program_options::variables_map arguments;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), arguments);
    boost::program_options::notify(arguments);
  } catch(boost::program_options::error const &error) {
    return startup_failure(error.what());
  }
  if(arguments.contains("help")) {
    std::cout << options << std::endl;
    return EXIT_SUCCESS;
  }
  if(arguments.contains("version")) {
    std::cout << "Corncob C++ 0.1.0 (reconstruction in progress)" << std::endl;
    return EXIT_SUCCESS;
  }
  bool const automatic_scale{!arguments.contains("scale")};
  int const scale{automatic_scale ? 1 : arguments["scale"].as<int>()};
  if(scale < 1 || scale > std::numeric_limits<int>::max() / corncob::render::screen_layout::display_width) {
    return startup_failure("--scale must be a positive integer whose window dimensions fit in an int");
  }
#ifndef NDEBUG
  double const seconds{arguments["seconds"].as<double>()};
  if(seconds < 0.0 || !std::isfinite(seconds)) return startup_failure("--seconds must be a finite non-negative number");
#endif // NDEBUG
  auto theatre_name{arguments["theatre"].as<std::string>()};
  std::unique_ptr<corncob::game::campaign_store> campaign;
  try {
    campaign = std::make_unique<corncob::game::campaign_store>(arguments["save-dir"].as<std::string>());
  } catch(std::exception const &error) { return startup_failure(error.what()); }
  corncob::world::theatre theatre;
  try {
    theatre = load_theatre(theatre_name);
    if(auto saved{campaign->load_world(theatre_name)}) {
      saved->sky = theatre.sky;
      saved->environment = theatre.environment;
      saved->briefing = std::move(theatre.briefing);
      saved->missions = std::move(theatre.missions);
      theatre = std::move(*saved);
    }
  } catch(std::exception const &error) {
    return startup_failure(error.what());
  }
  corncob::render::framebuffer frame;
  auto const cockpit{corncob::resources::decode_cockpit_mask(corncob::resources::cockpit_asset())};
  corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
  auto renderer{std::make_unique<corncob::graphics::scene_renderer>(geometry, cockpit)};
  auto game{std::make_unique<corncob::game::session>(geometry, std::move(theatre))};
  corncob::resources::collection interface_assets{corncob::resources::front_end_asset()};
  corncob::game::front_end front{interface_assets, *campaign, arguments.contains("skip-intro"), !arguments["theatre"].defaulted()};
  if(auto const *pilot{campaign->active_pilot()}) game->set_experimental_clearance(pilot->awards(corncob::game::medal::combat_master) != 0);
  auto observer{game->camera()};
#ifndef NDEBUG
  bool const no_cockpit{arguments.contains("no-cockpit")};
  if(no_cockpit) observer.cockpit = false;
  bool const cockpit_preview{arguments.contains("cockpit-preview")};
  bool const free_camera{arguments.contains("free-camera")};
  if(free_camera) {
    observer.position = corncob::maths::world_position{game->world().start};
    observer.cockpit = !no_cockpit;
  }
  if(cockpit_preview) corncob::resources::draw_cockpit_preview(cockpit, frame);
  else renderer->render(game->world(), observer, 0, 0, frame);
#else
  renderer->render(game->world(), observer, 0, 0, frame);
#endif // NDEBUG
#ifndef NDEBUG
  if(arguments.contains("screenshot")) {
    std::ofstream file{arguments["screenshot"].as<std::string>(), std::ios::binary};
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file << "P6\n" << corncob::render::screen_layout::width << ' ' << corncob::render::screen_layout::height << "\n255\n";
    for(auto const &pixel : frame.pixels) {
      file.put(static_cast<char>(pixel.red));
      file.put(static_cast<char>(pixel.green));
      file.put(static_cast<char>(pixel.blue));
    }
    file.close();
    return EXIT_SUCCESS;
  }
#endif // NDEBUG
  glfwSetErrorCallback([](int const code, char const *const message){
    std::cerr << "ERROR: GLFW " << code << ": " << message << std::endl;
  });
  if(!glfwInit()) return startup_failure("GLFW initialisation failed");
  boost::scope::scope_exit terminate_glfw{[]{
    glfwTerminate();
  }};
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  bool const compositor_sizing{automatic_scale && glfwGetPlatform() == GLFW_PLATFORM_WAYLAND};
  glfwWindowHint(GLFW_MAXIMIZED, compositor_sizing ? GLFW_TRUE : GLFW_FALSE);
  std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> const window{
    glfwCreateWindow(corncob::render::screen_layout::display_width * scale, corncob::render::screen_layout::display_height * scale,
                     "Corncob 3D", nullptr, nullptr),
    glfwDestroyWindow,
  };
  if(!window) return startup_failure("cannot create the Corncob window");
  if(automatic_scale && !compositor_sizing) {
    if(auto *monitor{glfwGetPrimaryMonitor()}) {
      int x{0}, y{0}, width{0}, height{0};
      glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
      int left{0}, top{0}, right{0}, bottom{0};
      glfwGetWindowFrameSize(window.get(), &left, &top, &right, &bottom);
      if(width > left + right && height > top + bottom) {
        auto const fitted{corncob::render::fit_viewport(width - left - right, height - top - bottom)};
        glfwSetWindowSize(window.get(), fitted.width, fitted.height);
        glfwSetWindowPos(window.get(), x + left + fitted.x, y + top + fitted.y);
      }
    }
  }
  glfwShowWindow(window.get());
  if(compositor_sizing) {
    int width{0}, height{0};
    glfwGetWindowSize(window.get(), &width, &height);
    glfwRestoreWindow(window.get());
    double const deadline{glfwGetTime() + 1.0};
    while(glfwGetWindowAttrib(window.get(), GLFW_MAXIMIZED) && glfwGetTime() < deadline) glfwWaitEventsTimeout(0.01);
    auto const fitted{corncob::render::fit_viewport(width, height)};
    glfwSetWindowSize(window.get(), fitted.width, fitted.height);
  }
  glfwMakeContextCurrent(window.get());
  glfwSwapInterval(1);
  bool sound_enabled{!arguments.contains("no-sound")};
  window_state state{game, front, sound_enabled};
  glfwSetWindowUserPointer(window.get(), &state);
  glfwSetKeyCallback(window.get(), [](GLFWwindow *const window, int const key, int const /*scancode*/, int const action, int const /*modifiers*/){
    if(action != GLFW_RELEASE) return;
    auto &state{*static_cast<window_state*>(glfwGetWindowUserPointer(window))};
    auto &game{state.game};
    auto &front{state.front};
    if(!front.in_flight()) {
      using enum corncob::game::interface_command;
      if(front.current_page() == corncob::game::interface_page::boss) { front.command(accept); return; }
      switch(key) {
      case GLFW_KEY_UP: front.command(up); break;
      case GLFW_KEY_DOWN: front.command(down); break;
      case GLFW_KEY_LEFT: front.command(previous); break;
      case GLFW_KEY_RIGHT: front.command(next); break;
      case GLFW_KEY_PAGE_UP: front.command(page_up); break;
      case GLFW_KEY_PAGE_DOWN: front.command(page_down); break;
      case GLFW_KEY_ENTER: case GLFW_KEY_KP_ENTER: front.command(accept); break;
      case GLFW_KEY_SPACE: if(!front.editing_name()) front.command(accept); break;
      case GLFW_KEY_BACKSPACE: front.command(backspace); break;
      case GLFW_KEY_ESCAPE: front.command(cancel); break;
      case GLFW_KEY_F4: front.command(help); break;
      default: break;
      }
      return;
    }
    if(game->in_map()) {
      if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_M || key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_SPACE) game->press(corncob::game::action::toggle_map);
      return;
    }
    switch(key) {
    case GLFW_KEY_M: game->press(corncob::game::action::toggle_map); break;
    case GLFW_KEY_V: game->press(corncob::game::action::call_rescue); break;
    case GLFW_KEY_ESCAPE: front.ask_to_end_sortie(); break;
    case GLFW_KEY_F1: front.show_boss(); break;
    case GLFW_KEY_F2: front.show_missions(game->world()); break;
    case GLFW_KEY_F3: front.show_current_score(game->results()); break;
    case GLFW_KEY_F4: front.show_flight_help(); break;
    case GLFW_KEY_S: state.sound_enabled = !state.sound_enabled; break;
    case GLFW_KEY_A: game->press(corncob::game::action::toggle_autopilot); break;
    case GLFW_KEY_R: game->press(corncob::game::action::toggle_rear); break;
    case GLFW_KEY_L: game->press(corncob::game::action::toggle_cockpit); break;
    case GLFW_KEY_NUM_LOCK: game->press(corncob::game::action::toggle_look); break;
    case GLFW_KEY_G: game->press(corncob::game::action::toggle_ground); break;
    case GLFW_KEY_F: game->press(corncob::game::action::toggle_flaps); break;
    case GLFW_KEY_I: game->press(corncob::game::action::rocket); break;
    case GLFW_KEY_E: game->press(corncob::game::action::eject); break;
    case GLFW_KEY_SPACE: game->press(corncob::game::action::parachute); break;
    case GLFW_KEY_CAPS_LOCK: game->press(corncob::game::action::toggle_remote); break;
    case GLFW_KEY_B: game->press(corncob::game::action::drop_bomb); break;
    case GLFW_KEY_SEMICOLON: game->press(corncob::game::action::plant_charge); break;
    case GLFW_KEY_D: game->press(corncob::game::action::arm_charges); break;
    case GLFW_KEY_P: game->press(corncob::game::action::pause); break;
    default: break;
    }
  });
  glfwSetCharCallback(window.get(), [](GLFWwindow *const window, unsigned int const codepoint){
    auto &state{*static_cast<window_state*>(glfwGetWindowUserPointer(window))};
    state.front.character(codepoint);
  });
  corncob::audio::effects sound{interface_assets};
  std::unique_ptr<corncob::platform::audio_output> audio_output;
  corncob::platform::framebuffer_presenter presenter{*window};
  unsigned int frame_number{0};
  double previous_time{glfwGetTime()}, accumulated_time{0.0};
  double constexpr simulation_period{40.0 * 1024.0 / 1'193'182.0};
  while(!glfwWindowShouldClose(window.get())) {
    glfwPollEvents();
    if(front.quitting()) break;
    if(sound_enabled && !audio_output) {
      try {
        audio_output = std::make_unique<corncob::platform::audio_output>([](void *const userdata, std::span<float> const samples) noexcept {
          static_cast<corncob::audio::effects*>(userdata)->render(samples);
        }, &sound);
      } catch(std::exception const &error) {
        std::cerr << "WARNING: sound effects unavailable: " << error.what() << std::endl;
        sound_enabled = false;
      }
    }
    if(auto const launch{front.take_launch()}) {
      sound.reset();
      theatre_name = launch->theatre;
      auto world{launch->training ? corncob::world::generate_training(geometry, *launch->training) : load_theatre(theatre_name)};
      if(auto saved{launch->training ? std::nullopt : campaign->load_world(theatre_name)}) {
        saved->sky = world.sky;
        saved->environment = world.environment;
        saved->briefing = std::move(world.briefing);
        saved->missions = std::move(world.missions);
        world = std::move(*saved);
      }
      game = std::make_unique<corncob::game::session>(geometry, std::move(world));
      if(auto const *pilot{campaign->active_pilot()}) game->set_experimental_clearance(pilot->awards(corncob::game::medal::combat_master) != 0);
      renderer = std::make_unique<corncob::graphics::scene_renderer>(geometry, cockpit);
      if(launch->training) game->set_invincible(launch->training->invincible);
      front.begin_flight();
      accumulated_time = 0.0;
    }
    if(auto const relocation{front.take_relocation()}) game->relocate(*relocation);
    double const now{glfwGetTime()};
    double const elapsed{std::min(now - previous_time, 0.1)};
    previous_time = now;
    front.advance(elapsed);
#ifndef NDEBUG
    if(seconds > 0.0 && now >= seconds) break;
    if(free_camera && glfwGetWindowAttrib(window.get(), GLFW_FOCUSED)) {
      auto held{[&window](int const key){
        return glfwGetKey(window.get(), key) == GLFW_PRESS;
      }};
      int const turn{static_cast<int>(elapsed * 10'000.0)};
      observer.attitude.heading += static_cast<corncob::maths::angle>((held(GLFW_KEY_LEFT) - held(GLFW_KEY_RIGHT)) * turn);
      observer.attitude.pitch += static_cast<corncob::maths::angle>((held(GLFW_KEY_DOWN) - held(GLFW_KEY_UP)) * turn);
      auto const axes{corncob::maths::make_rotation(observer.attitude)};
      double const speed{elapsed * (held(GLFW_KEY_LEFT_SHIFT) ? 200'000.0 : 25'000.0)};
      for(unsigned int axis{0}; axis != 3; ++axis) {
        double const movement{((held(GLFW_KEY_W) - held(GLFW_KEY_S)) * axes.rows[0][axis]
          + (held(GLFW_KEY_A) - held(GLFW_KEY_D)) * axes.rows[1][axis]) * speed / 32'768.0};
        observer.position[axis] += static_cast<int32_t>(movement);
      }
      observer.position.z += static_cast<int32_t>((held(GLFW_KEY_PAGE_UP) - held(GLFW_KEY_PAGE_DOWN)) * speed);
      if(held(GLFW_KEY_HOME)) {
        observer.position = corncob::maths::world_position{game->world().start};
        observer.attitude = {
          .heading{static_cast<corncob::maths::angle>(-8000)},
          .pitch{static_cast<corncob::maths::angle>(-1500)},
          .roll{0},
        };
      }
    }
#endif // NDEBUG
    bool inspect{false};
    corncob::game::input_state input;
#ifndef NDEBUG
    inspect = free_camera || cockpit_preview;
#endif // NDEBUG
    if(!inspect && front.in_flight()) {
      if(glfwGetWindowAttrib(window.get(), GLFW_FOCUSED)) {
        auto held{[&window](int const key){ return glfwGetKey(window.get(), key) == GLFW_PRESS; }};
        input = {
          .stick{
            .roll{static_cast<int16_t>(((held(GLFW_KEY_RIGHT) || held(GLFW_KEY_KP_6)) - (held(GLFW_KEY_LEFT) || held(GLFW_KEY_KP_4))) * 512)},
            .pitch{static_cast<int16_t>(((held(GLFW_KEY_DOWN) || held(GLFW_KEY_KP_2)) - (held(GLFW_KEY_UP) || held(GLFW_KEY_KP_8))) * 512)},
            .rudder{static_cast<int16_t>(((held(GLFW_KEY_Z) || held(GLFW_KEY_KP_7)) - (held(GLFW_KEY_X) || held(GLFW_KEY_KP_9))) * 512)},
            .brakes{held(GLFW_KEY_PERIOD) || held(GLFW_KEY_KP_0)},
          },
          .throttle_up{held(GLFW_KEY_EQUAL) || held(GLFW_KEY_KP_ADD)},
          .throttle_down{held(GLFW_KEY_MINUS) || held(GLFW_KEY_KP_SUBTRACT)},
          .walk_forward{held(GLFW_KEY_KP_5)},
          .walk_backward{held(GLFW_KEY_KP_0) || held(GLFW_KEY_INSERT)},
          .jump{held(GLFW_KEY_J)},
          .drop_through{held(GLFW_KEY_E)},
          .advance_time{held(GLFW_KEY_T)},
          .weapons{
            .gun{held(GLFW_KEY_SPACE)},
            .cannon{held(GLFW_KEY_LEFT_SHIFT)},
            .missile{held(GLFW_KEY_C) && !held(GLFW_KEY_LEFT_CONTROL) && !held(GLFW_KEY_RIGHT_CONTROL)},
          },
          .view{
            .left{held(GLFW_KEY_LEFT) || held(GLFW_KEY_KP_4)},
            .right{held(GLFW_KEY_RIGHT) || held(GLFW_KEY_KP_6)},
            .up{held(GLFW_KEY_UP) || held(GLFW_KEY_KP_8)},
            .down{held(GLFW_KEY_DOWN) || held(GLFW_KEY_KP_2)},
            .below{held(GLFW_KEY_KP_5)},
            .modifier{held(GLFW_KEY_LEFT_CONTROL) || held(GLFW_KEY_RIGHT_CONTROL)},
          },
          .map{
            .higher{held(GLFW_KEY_PAGE_DOWN) || held(GLFW_KEY_KP_3)},
            .lower{held(GLFW_KEY_PAGE_UP) || held(GLFW_KEY_KP_9)},
            .home{held(GLFW_KEY_HOME) || held(GLFW_KEY_KP_7)},
            .rotate{held(GLFW_KEY_LEFT_SHIFT)},
            .slow{held(GLFW_KEY_S)},
          },
        };
      }
      accumulated_time += elapsed;
      while(accumulated_time >= simulation_period) {
        game->advance(input);
        accumulated_time -= simulation_period;
      }
      observer = game->camera();
#ifndef NDEBUG
      if(no_cockpit) observer.cockpit = false;
#endif // NDEBUG
      if(frame_number % 15 == 0) {
        std::string const title{"Corncob 3D - " + game->message()};
        glfwSetWindowTitle(window.get(), title.c_str());
      }
    }
    auto audio_frame{corncob::audio::observe(*game)};
    audio_frame.active = sound_enabled && front.in_flight() && !game->is_paused() && !game->in_map() && !inspect;
    audio_frame.dead = audio_frame.dead && sound_enabled;
    sound.update(audio_frame);
    if(!inspect && (front.take_finish() || (front.in_flight() && game->finished()))) {
      auto const result{game->results()};
      campaign->finish_sortie(theatre_name, game->snapshot(), result);
      front.finish_sortie(result);
      if(result.has(corncob::game::sortie_status::earth_saved)) front.begin_ending(renderer->indexed_frame(), renderer->colours());
    }
    if(!front.in_flight() && !inspect) {
      accumulated_time = 0.0;
      front.render(frame);
      presenter.present(frame);
      ++frame_number;
      glfwWaitEventsTimeout(1.0 / 60.0);
      continue;
    }
    corncob::graphics::instrument_telemetry const telemetry{
      .aircraft{game->craft()},
      .controls{input.stick},
      .paused{game->is_paused()},
      .completed{game->statistics().objectives_completed != 0},
      .autopilot{game->autopilot_engaged()},
      .damage{&game->aircraft_damage()},
    };
    auto const *const panel{!inspect && !game->in_map() && game->player() == corncob::game::player_mode::in_aircraft ? &telemetry : nullptr};
#ifndef NDEBUG
    if(!cockpit_preview) renderer->render(game->world(), observer, inspect ? frame_number : game->frame_number(),
      static_cast<uint16_t>(inspect ? static_cast<uint64_t>(now * 1'193'182.0 / 1024.0) : game->timer()), frame, panel);
#else
    renderer->render(game->world(), observer, game->frame_number(), static_cast<uint16_t>(game->timer()), frame, panel);
#endif // NDEBUG
    ++frame_number;
    presenter.present(frame);
    glfwWaitEventsTimeout(1.0 / 60.0);
  }
  return EXIT_SUCCESS;
}

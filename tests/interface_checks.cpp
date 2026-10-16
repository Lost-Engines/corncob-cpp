#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "game/front_end.h"
#include "resources/assets.h"

namespace {

void capture(corncob::render::framebuffer const &frame, std::filesystem::path const &path) {
  /// Optional development captures retain each original video mode's actual pixel dimensions
  std::ofstream file{path, std::ios::binary};
  file.exceptions(std::ios::badbit | std::ios::failbit);
  file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
  for(unsigned int i{0}; i != frame.width * frame.height; ++i) {
    auto const &pixel{frame.pixels[i]};
    file.put(static_cast<char>(pixel.red));
    file.put(static_cast<char>(pixel.green));
    file.put(static_cast<char>(pixel.blue));
  }
}

} // anonymous namespace

int main(int const argc, char const *const argv[]) {
  /// Decode every original composition and traverse title, briefing, help, confirmation and outcome chains without a window
  namespace game = corncob::game;
  corncob::resources::collection assets{corncob::resources::front_end_asset()};
  auto const stories{assets.story()}, outcomes{assets.outcomes()};
  if(stories.size() != 2 || outcomes.size() != 15) throw std::runtime_error{"original presentation count changed"};
  corncob::render::framebuffer frame;
  if(argc < 2 || argc > 3) throw std::invalid_argument{"interface_checks requires its dedicated scratch directory and optional capture directory"};
  std::filesystem::path const scratch{argv[1]};
  if(scratch.filename() != "interface-checks") throw std::invalid_argument{"interface check directory must be named interface-checks"};
  std::filesystem::remove_all(scratch);
  game::campaign_store career{scratch};
  game::front_end front{assets, career};
  front.advance(0.4);
  front.render(frame);
  if(frame.width != 320 || frame.height != 200) throw std::runtime_error{"title lost its original VGA resolution"};
  if(argc == 3) std::filesystem::create_directories(argv[2]);
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "title.ppm");
  for(unsigned int i{0}; i != stories.size(); ++i) {
    corncob::graphics::draw_presentation(frame, assets, stories[i]);
    if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / ("story" + std::to_string(i) + ".ppm"));
  }
  for(unsigned int i{0}; i != outcomes.size(); ++i) {
    front.show_outcome(i);
    front.render(frame);
    if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / ("outcome" + std::to_string(i) + ".ppm"));
  }
  front.command(game::interface_command::accept);
  front.render(frame);
  if(frame.width != 640 || frame.height != 400) throw std::runtime_error{"menu lost its EGA text resolution"};
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "menu.ppm");
  front.command(game::interface_command::accept);
  front.command(game::interface_command::accept);
  for(unsigned int c : std::string{"Test Pilot"}) front.character(c);
  front.command(game::interface_command::accept);
  if(!career.active_pilot() || career.active_pilot()->name() != "Test Pilot") throw std::runtime_error{"roster name entry failed"};
  front.command(game::interface_command::cancel);
  for(unsigned int i{0}; i != 3; ++i) front.command(game::interface_command::down);
  front.command(game::interface_command::accept);
  front.command(game::interface_command::next);
  if(front.current_page() != game::interface_page::medals) throw std::runtime_error{"pilot medal view was not reachable"};
  front.render(frame);
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "medals.ppm");
  auto const menu_font{assets.menu_font()};
  auto const medals{assets.file("medal.fnc")};
  for(unsigned int offset{0}; offset != medals.size(); offset += 17)
    for(unsigned int row{0}; row != 16; ++row)
      if(menu_font[medals[offset + 16] * 16u + row] != medals[offset + row]) throw std::runtime_error{"medal glyph differs from its original font record"};
  front.command(game::interface_command::previous);
  if(front.current_page() != game::interface_page::records) throw std::runtime_error{"medal view did not return to career records"};
  front.command(game::interface_command::cancel);
  front.command(game::interface_command::cancel);
  front.command(game::interface_command::down);
  front.command(game::interface_command::accept);
  front.command(game::interface_command::accept);
  front.render(frame);
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "briefing.ppm");
  front.command(game::interface_command::accept);
  auto const launch{front.take_launch()};
  if(!launch || launch->theatre != "morning" || front.take_launch()) throw std::runtime_error{"menu launch request lost or repeated"};
  front.begin_flight();
  front.show_flight_help();
  for(unsigned int page{0}; page != 10; ++page) {
    front.render(frame);
    if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / ("help" + std::to_string(page) + ".ppm"));
    front.command(game::interface_command::accept);
  }
  if(!front.in_flight()) throw std::runtime_error{"help sequence did not return to flight"};
  for(unsigned int notice{0}; notice != static_cast<unsigned int>(std::to_underlying(corncob::resources::radio_notice::count)); ++notice) {
    auto const type{static_cast<corncob::resources::radio_notice>(notice)};
    auto const message{corncob::resources::read_radio_message(type)};
    if(message.lines.empty() || message.border.left < 0 || message.border.left + message.border.width > 640) throw std::runtime_error{"original radio message has invalid placement"};
    front.show_radio(type, frame, type == corncob::resources::radio_notice::map_instructions);
    front.render(frame);
    if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / ("radio" + std::to_string(notice) + ".ppm"));
    front.command(game::interface_command::accept);
    if(!front.in_flight() || front.take_map() != (type == corncob::resources::radio_notice::map_instructions)) throw std::runtime_error{"radio acknowledgement lost flight/map continuation"};
  }
  front.ask_to_end_sortie();
  front.command(game::interface_command::cancel);
  if(!front.in_flight()) throw std::runtime_error{"quit cancellation lost the flight"};
  front.ask_to_end_sortie();
  front.command(game::interface_command::up);
  front.command(game::interface_command::accept);
  if(!front.take_finish() || front.take_finish()) throw std::runtime_error{"end-sortie request lost or repeated"};
  front.abort_sortie();
  if(!front.take_finish() || !front.take_abort() || front.take_abort()) throw std::runtime_error{"abort status was lost or repeated"};
  game::front_end setup{assets, career, true};
  for(unsigned int i{0}; i != 5; ++i) setup.command(game::interface_command::down);
  setup.command(game::interface_command::accept);
  setup.command(game::interface_command::next);
  setup.command(game::interface_command::down);
  setup.command(game::interface_command::next);
  if(setup.flight_controls().joystick_device != -2 || setup.flight_controls().beginner_flight) throw std::runtime_error{"control setup did not change keyboard selection and aerodynamic model"};
  setup.render(frame);
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "control-setup.ppm");
  setup.command(game::interface_command::down);
  setup.command(game::interface_command::accept);
  if(setup.current_page() != game::interface_page::help) throw std::runtime_error{"control setup lost the original help sequence"};
  setup.command(game::interface_command::cancel);
  if(setup.current_page() != game::interface_page::control_setup) throw std::runtime_error{"help did not return to control setup"};
  game::campaign_store reopened{scratch};
  game::front_end restored{assets, reopened, true};
  if(restored.flight_controls().joystick_device != -2 || restored.flight_controls().beginner_flight) throw std::runtime_error{"control menu settings were lost on restart"};
  restored.command(game::interface_command::accept);
  for(unsigned int i{0}; i != 6; ++i) restored.command(game::interface_command::down);
  restored.command(game::interface_command::accept);
  restored.command(game::interface_command::accept);
  restored.render(frame);
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "delete-pilot.ppm");
  if(restored.current_page() != game::interface_page::pilot_delete) throw std::runtime_error{"pilot deletion confirmation was not reachable"};
  restored.command(game::interface_command::accept);
  if(reopened.pilots().size() != 1) throw std::runtime_error{"pilot deletion did not default to cancellation"};
  for(unsigned int i{0}; i != 6; ++i) restored.command(game::interface_command::down);
  restored.command(game::interface_command::accept);
  restored.command(game::interface_command::accept);
  restored.command(game::interface_command::up);
  restored.command(game::interface_command::accept);
  if(!reopened.pilots().empty()) throw std::runtime_error{"confirmed pilot deletion did not remove the pilot"};
  game::front_end intro{assets, reopened};
  for(unsigned int i{0}; i != 2; ++i) {
    intro.advance(0.4);
    intro.command(game::interface_command::accept);
    intro.advance(0.4);
  }
  if(intro.current_page() != game::interface_page::title) throw std::runtime_error{"story fades did not lead to the animated title"};
  intro.advance(4.0);
  intro.render(frame);
  if(argc == 3) capture(frame, std::filesystem::path{argv[2]} / "animated-title.ppm");
  intro.command(game::interface_command::accept);
  if(intro.current_page() != game::interface_page::menu) throw std::runtime_error{"animated title could not continue to the menu"};
  std::filesystem::remove_all(scratch);
  std::cout << "Two story panels, fifteen outcomes, ten help pages and native menu navigation exercised" << std::endl;
  return EXIT_SUCCESS;
}

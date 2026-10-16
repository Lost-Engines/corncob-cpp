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
  std::filesystem::path const scratch{std::filesystem::temp_directory_path() / "corncob-interface-checks"};
  std::filesystem::remove_all(scratch);
  game::campaign_store career{scratch};
  game::front_end front{assets, career};
  front.render(frame);
  if(frame.width != 320 || frame.height != 200) throw std::runtime_error{"title lost its original VGA resolution"};
  if(argc == 2) std::filesystem::create_directories(argv[1]);
  if(argc == 2) capture(frame, std::filesystem::path{argv[1]} / "title.ppm");
  for(unsigned int i{0}; i != stories.size(); ++i) {
    corncob::graphics::draw_presentation(frame, assets, stories[i]);
    if(argc == 2) capture(frame, std::filesystem::path{argv[1]} / ("story" + std::to_string(i) + ".ppm"));
  }
  for(unsigned int i{0}; i != outcomes.size(); ++i) {
    front.show_outcome(i);
    front.render(frame);
    if(argc == 2) capture(frame, std::filesystem::path{argv[1]} / ("outcome" + std::to_string(i) + ".ppm"));
  }
  front.command(game::interface_command::accept);
  front.render(frame);
  if(frame.width != 640 || frame.height != 400) throw std::runtime_error{"menu lost its EGA text resolution"};
  if(argc == 2) capture(frame, std::filesystem::path{argv[1]} / "menu.ppm");
  front.command(game::interface_command::accept);
  front.command(game::interface_command::accept);
  for(unsigned int c : std::string{"Test Pilot"}) front.character(c);
  front.command(game::interface_command::accept);
  if(!career.active_pilot() || career.active_pilot()->name() != "Test Pilot") throw std::runtime_error{"roster name entry failed"};
  front.command(game::interface_command::cancel);
  front.command(game::interface_command::down);
  front.command(game::interface_command::accept);
  front.command(game::interface_command::accept);
  front.render(frame);
  if(argc == 2) capture(frame, std::filesystem::path{argv[1]} / "briefing.ppm");
  front.command(game::interface_command::accept);
  auto const launch{front.take_launch()};
  if(!launch || launch->theatre != "morning" || front.take_launch()) throw std::runtime_error{"menu launch request lost or repeated"};
  front.begin_flight();
  front.show_flight_help();
  for(unsigned int page{0}; page != 10; ++page) {
    front.render(frame);
    if(argc == 2) capture(frame, std::filesystem::path{argv[1]} / ("help" + std::to_string(page) + ".ppm"));
    front.command(game::interface_command::accept);
  }
  if(!front.in_flight()) throw std::runtime_error{"help sequence did not return to flight"};
  front.ask_to_end_sortie();
  front.command(game::interface_command::cancel);
  if(!front.in_flight()) throw std::runtime_error{"quit cancellation lost the flight"};
  front.ask_to_end_sortie();
  front.command(game::interface_command::up);
  front.command(game::interface_command::accept);
  if(!front.take_finish() || front.take_finish()) throw std::runtime_error{"end-sortie request lost or repeated"};
  std::filesystem::remove_all(scratch);
  std::cout << "Two story panels, fifteen outcomes, ten help pages and native menu navigation exercised" << std::endl;
  return EXIT_SUCCESS;
}

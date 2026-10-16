#include "front_end.h"
#include <algorithm>
#include <sstream>
#include <utility>
#include "resources/assets.h"
#include "world/world.h"

namespace corncob::game {

namespace {

std::array<std::string_view, 4> constexpr theatre_titles{"Atkinsville (easy)", "Dusk (medium)", "Wasteland (hard)", "White Sands testing ground"};
std::array<std::string_view, 4> constexpr theatre_names{"morning", "dusk", "wasteland", "white_sands"};
render::rgba_pixel constexpr white{255, 255, 255}, grey{170, 170, 170}, yellow{255, 255, 85}, blue{0, 0, 170};
std::span<uint8_t const> briefing_data(std::string_view const name) {
  /// Theatre descriptions and mission intelligence come from the original TWR text
  if(name == "morning") return resources::morning_briefing_asset();
  if(name == "dusk") return resources::dusk_briefing_asset();
  if(name == "wasteland") return resources::wasteland_briefing_asset();
  return resources::white_sands_briefing_asset();
}

} // anonymous namespace

front_end::front_end(resources::collection const &assets, bool const skip_intro, bool const start_in_flight) :
  assets{assets},
  stories{assets.story()},
  outcomes{assets.outcomes()} {
  /// One state machine replaces the original title, menu, help and outcome helper processes
  page = start_in_flight ? interface_page::flight : skip_intro ? interface_page::menu : interface_page::title;
  for(auto const name : {"img/3d2.img", "img/3d3.img", "img/3d4.img", "img/3d5.img", "img/3d6.img",
    "img/3d7.img", "img/3d8.img", "img/3d9.img", "img/3d10.img", "img/3d13.img"}) help_screens.push_back(resources::decode_cockpit_mask(assets.file(name)));
}
void front_end::accept() {
  /// Advance coherent title/story/menu chains without launching DOS helpers
  switch(page) {
  case interface_page::title:
    page = interface_page::story;
    story_index = 0;
    break;
  case interface_page::story:
    if(++story_index == stories.size()) page = interface_page::menu;
    break;
  case interface_page::menu:
    if(session_running && selection == 0) page = interface_page::flight;
    else if(selection == (session_running ? 1u : 0u)) {
      page = interface_page::theatres;
      selection = 0;
    } else if(selection == (session_running ? 2u : 1u)) {
      return_page = page;
      page = interface_page::help;
      help_index = 0;
    } else if(selection == (session_running ? 3u : 2u)) {
      page = interface_page::story;
      story_index = 0;
    } else {
      page = interface_page::quit_confirmation;
      selection = 0;
    }
    break;
  case interface_page::theatres:
    {
      selected_theatre = theatre_names[selection];
      auto const data{briefing_data(selected_theatre)};
      briefing.assign(reinterpret_cast<char const*>(data.data()), data.size());
      if(!briefing.empty() && briefing.front() == 'X') briefing.erase(0, briefing.find('\n') + 1);
      scroll = 0;
      page = interface_page::briefing;
    }
    break;
  case interface_page::briefing:
    pending_launch = launch_request{
      .theatre{selected_theatre},
    };
    break;
  case interface_page::help:
    if(++help_index == help_screens.size()) page = return_page;
    break;
  case interface_page::quit_confirmation:
    if(selection == 0) {
      if(session_running) {
        session_running = false;
        page = interface_page::menu;
      } else quit_requested = true;
    } else page = session_running ? interface_page::flight : interface_page::menu;
    selection = 0;
    break;
  case interface_page::outcome:
    page = interface_page::menu;
    session_running = false;
    selection = 0;
    break;
  case interface_page::flight:
    break;
  }
}
void front_end::command(interface_command const command) {
  /// Keyboard navigation operates on the current page, keeping flight input separate while a menu is open
  unsigned int const count{page == interface_page::theatres ? 4u : page == interface_page::menu ? (session_running ? 5u : 4u) : 2u};
  switch(command) {
  case interface_command::accept: accept(); break;
  case interface_command::up:
    if(page == interface_page::briefing) { if(scroll != 0) --scroll; }
    else selection = (selection + count - 1) % count;
    break;
  case interface_command::down:
    if(page == interface_page::briefing) ++scroll;
    else selection = (selection + 1) % count;
    break;
  case interface_command::page_down:
    if(page == interface_page::briefing) scroll += 17;
    break;
  case interface_command::page_up:
    scroll = scroll > 17 ? scroll - 17 : 0;
    break;
  case interface_command::previous:
    if(page == interface_page::help && help_index != 0) --help_index;
    break;
  case interface_command::next:
    if(page == interface_page::help || page == interface_page::story || page == interface_page::title) accept();
    break;
  case interface_command::cancel:
    if(page == interface_page::flight) ask_to_end_sortie();
    else if(page == interface_page::help) page = return_page;
    else if(page == interface_page::quit_confirmation) page = session_running ? interface_page::flight : interface_page::menu;
    else if(page == interface_page::title || page == interface_page::story) page = interface_page::menu;
    else if(page == interface_page::menu && session_running) page = interface_page::flight;
    else if(page == interface_page::menu) {
      page = interface_page::quit_confirmation;
      selection = 0;
    } else {
      page = interface_page::menu;
      selection = 0;
    }
    break;
  case interface_command::help:
    return_page = page;
    page = interface_page::help;
    help_index = 0;
    break;
  }
}
void front_end::show_menu() {
  /// The native helper flow can return to a retained flight without recreating its world
  page = interface_page::menu;
  selection = 0;
}
void front_end::begin_flight() {
  /// A newly created native session owns the selected theatre until its sortie ends
  session_running = true;
  page = interface_page::flight;
}
void front_end::show_flight_help() {
  /// F4 opens the original in-flight help sequence and returns to the same session
  session_running = true;
  return_page = interface_page::flight;
  page = interface_page::help;
  help_index = 0;
}
void front_end::ask_to_end_sortie() {
  /// Escape requires a deliberate confirmation and keeps the current flight available on cancellation
  session_running = true;
  page = interface_page::quit_confirmation;
  selection = 1;
}
void front_end::show_outcome(unsigned int const index) {
  /// Outcome selection comes from sortie state; rendering retains the original stored composition
  outcome_index = std::min(index, static_cast<unsigned int>(outcomes.size() - 1));
  page = interface_page::outcome;
}
void front_end::draw_menu(graphics::interface_canvas &canvas, std::string_view const title, std::span<std::string_view const> const choices) const {
  /// Retain a simple BIOS-font interface, with selection and instructions visible at the same time
  canvas.clear();
  canvas.rectangle(4, 20, 636, 392, blue);
  canvas.box(0, 16, 640, 396, yellow);
  canvas.box(2, 18, 638, 394, white);
  canvas.rectangle(168, 0, 464, 76, blue);
  canvas.box(172, 4, 460, 72, white);
  canvas.text(272, 12, "Corncob 3D", white);
  canvas.text(316 - static_cast<int>(title.size()) * 4, 44, title, grey);
  canvas.text(80, 92, "Use arrow keys and Enter to choose an action:", {85, 255, 255});
  for(unsigned int n{0}; n != choices.size(); ++n) {
    int const y{124 + static_cast<int>(n) * 24};
    if(n == selection) canvas.rectangle(96, y, 600, y + 16, {0, 0, 85});
    canvas.text(112, y, choices[n], n == selection ? yellow : grey);
  }
  canvas.text(72, 348, "Arrows select   Enter accepts   Esc returns", grey);
  canvas.text(72, 368, "F4: flight controls and help", grey);
}
void front_end::render(render::framebuffer &target) const {
  /// Menus and helper-program artwork share the native window but retain their original video-mode pixel sizes
  if(page == interface_page::story) {
    graphics::draw_presentation(target, assets, stories.at(story_index));
    return;
  }
  if(page == interface_page::outcome) {
    graphics::draw_presentation(target, assets, outcomes.at(outcome_index));
    return;
  }
  if(page == interface_page::help) {
    resources::draw_cockpit_preview(help_screens.at(help_index), target);
    target.width = 640;
    target.height = 350;
    return;
  }
  if(page == interface_page::title) {
    graphics::interface_canvas canvas{target, assets.file("font8.bin"), 8, 320, 200};
    canvas.clear();
    auto const top{assets.image("vga/tlogogr.vga")}, bottom{assets.image("vga/blogogr.vga")};
    canvas.image(top, static_cast<int>((320 - top.width) / 2), 8);
    canvas.image(bottom, static_cast<int>((320 - bottom.width) / 2), 200 - static_cast<int>(bottom.height) - 32);
    canvas.text(40, 184, "Press Enter to continue", white);
    return;
  }
  graphics::interface_canvas canvas{target, assets.file("font16.bin"), 16, 640, 400};
  if(page == interface_page::menu) {
    static std::array<std::string_view, 4> constexpr choices{"Combat missions", "Flight controls and help", "Introduction", "Quit"};
    static std::array<std::string_view, 5> constexpr resume{"Resume flight", "Combat missions", "Flight controls and help", "Introduction", "End current sortie"};
    draw_menu(canvas, "Main menu", session_running ? std::span<std::string_view const>{resume} : std::span<std::string_view const>{choices});
  } else if(page == interface_page::theatres) draw_menu(canvas, "Choose a theatre of operations", theatre_titles);
  else if(page == interface_page::quit_confirmation) {
    static std::array<std::string_view, 2> constexpr choices{"Yes", "No - return"};
    draw_menu(canvas, session_running ? "End this sortie?" : "Quit Corncob 3D?", choices);
  } else if(page == interface_page::briefing) {
    canvas.clear();
    canvas.rectangle(0, 0, 640, 28, blue);
    canvas.text(16, 6, "Airbase intelligence", white);
    std::istringstream input{briefing};
    std::string line;
    unsigned int number{0}, drawn{0};
    while(std::getline(input, line) && drawn != 20) {
      if(!line.empty() && line.back() == '\r') line.pop_back();
      if(line.find('\f') != std::string::npos || line.starts_with("Sxyth")) continue;
      if(number++ < scroll) continue;
      if(line.size() > 78) line.resize(78);
      canvas.text(8, 34 + static_cast<int>(drawn++) * 14, line, grey);
    }
    canvas.text(8, 326, "Up/Down or PgUp/PgDn scroll   Enter starts   Esc returns", yellow);
  }
}
std::optional<launch_request> front_end::take_launch() {
  /// Consume launch requests once so the main loop can replace its owned session safely
  return std::exchange(pending_launch, std::nullopt);
}
bool front_end::in_flight() const noexcept {
  /// Simulation and flight controls run only while the flight page owns input
  return page == interface_page::flight;
}
bool front_end::quitting() const noexcept {
  /// Keep window-system shutdown outside the core menu state machine
  return quit_requested;
}
interface_page front_end::current_page() const noexcept {
  /// Expose page state for scripted UI traversal without coupling to GLFW
  return page;
}

} // namespace corncob::game

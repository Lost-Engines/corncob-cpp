#include "front_end.h"
#include "render/palette.h"
#include <algorithm>
#include <sstream>
#include <format>
#include <utility>
#include "resources/assets.h"
#include "world/world.h"

namespace corncob::game {

namespace {

std::array<std::string_view, 4> constexpr theatre_titles{"Atkinsville (easy)", "Dusk (medium)", "Wasteland (medium)", "White Sands Testing Ground"};
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

front_end::front_end(resources::collection const &assets, campaign_store &campaign, bool const skip_intro, bool const start_in_flight, std::optional<control_setup> const controls, std::optional<bool> const music_enabled) :
  assets{assets},
  campaign{campaign},
  menu_font{assets.menu_font()},
  training{campaign.training_settings()},
  controls{controls.value_or(control_setup{
    .joystick_device{campaign.keyboard_control() ? -2 : -1},
    .beginner_flight{!campaign.active_pilot() || campaign.active_pilot()->beginner_flight()},
  })},
  stories{assets.story()},
  outcomes{assets.outcomes()},
  music_on{music_enabled.value_or(campaign.music_enabled())} {
  /// One state machine replaces the original title, menu, help and outcome helper processes
  page = start_in_flight ? interface_page::flight : skip_intro ? interface_page::menu : interface_page::story;
  for(auto const name : {"img/3d2.img", "img/3d3.img", "img/3d4.img", "img/3d5.img", "img/3d6.img",
    "img/3d7.img", "img/3d8.img", "img/3d9.img", "img/3d10.img", "img/3d13.img"}) help_screens.push_back(resources::decode_cockpit_mask(assets.file(name)));
}
void front_end::accept() {
  /// Advance coherent title/story/menu chains without launching DOS helpers
  switch(page) {
  case interface_page::radio:
    radio.reset();
    page = interface_page::flight;
    pending_map = std::exchange(map_after_radio, false);
    break;
  case interface_page::ending:
    ending->toggle_warp();
    break;
  case interface_page::title:
    title.reset();
    page = interface_page::menu;
    selection = 0;
    break;
  case interface_page::story:
    advancing_story = true;
    break;
  case interface_page::menu:
    feedback.clear();
    if(session_running) {
      if(selection == 0) page = interface_page::flight;
      else if(selection == 1) show_flight_help();
      else ask_to_end_sortie();
    } else if((selection == 1 || selection == 2) && !campaign.can_fly()) {
      feedback = campaign.active_pilot()->unavailable() ? "Activate a living pilot or resurrect this pilot first"
        : "Pilot suspended after friendly losses - select another pilot";
    } else switch(selection) {
    case 0: page = interface_page::roster; break;
    case 1: page = interface_page::theatres; break;
    case 2: page = interface_page::training; break;
    case 3:
      if(campaign.active_pilot()) {
        auto const pilots{campaign.pilots()};
        pilot_index = static_cast<unsigned int>(campaign.active_pilot() - pilots.data());
        page = interface_page::records;
      } else feedback = "Create or activate a pilot in the duty roster first";
      break;
    case 4: page = interface_page::high_scores; score_list = 0; break;
    case 5: page = interface_page::control_setup; break;
    case 6: page = interface_page::story; story_index = 0; story_intensity = 0.0; advancing_story = false; break;
    default: page = interface_page::quit_confirmation; break;
    }
    selection = 0;
    break;
  case interface_page::roster:
    feedback.clear();
    if(selection == 0) {
      renaming = false;
      entered_name.clear();
      page = interface_page::pilot_name;
    } else if(selection <= 4 || selection == 6) {
      if(campaign.pilots().empty()) feedback = "The duty roster is empty - create a pilot first";
      else {
        pilot_action = selection == 1 ? roster_action::activate : selection == 2 ? roster_action::records
          : selection == 3 ? roster_action::rename : selection == 4 ? roster_action::resurrect : roster_action::remove;
        page = interface_page::pilots;
      }
    } else if(selection == 5) {
      campaign.deactivate();
      feedback = "Trainee active - sorties will not be saved";
    } else page = interface_page::menu;
    selection = 0;
    break;
  case interface_page::pilots:
    if(selection < campaign.pilots().size()) {
      pilot_index = selection;
      try {
        switch(pilot_action) {
        case roster_action::activate: campaign.activate(pilot_index); training = campaign.training_settings(); controls.beginner_flight = campaign.active_pilot()->beginner_flight(); page = interface_page::roster; break;
        case roster_action::remove: page = interface_page::pilot_delete; break;
        case roster_action::records: page = interface_page::records; break;
        case roster_action::rename:
          renaming = true;
          entered_name = campaign.pilots()[pilot_index].name();
          page = interface_page::pilot_name;
          break;
        case roster_action::resurrect: campaign.resurrect(pilot_index); page = interface_page::roster; break;
        }
        feedback.clear();
      } catch(std::exception const &error) { feedback = error.what(); }
    }
    selection = page == interface_page::pilot_delete ? 1 : 0;
    break;
  case interface_page::pilot_delete:
    if(selection == 0) {
      try { campaign.delete_pilot(pilot_index); feedback = "Pilot deleted; achievements retained in high scores"; }
      catch(std::exception const &error) { feedback = error.what(); }
    }
    page = interface_page::roster;
    selection = 0;
    break;
  case interface_page::pilot_name:
    try {
      if(renaming) campaign.rename_pilot(pilot_index, entered_name);
      else { campaign.create_pilot(entered_name); training = campaign.training_settings(); controls.beginner_flight = campaign.active_pilot()->beginner_flight(); }
      feedback.clear();
      page = interface_page::roster;
      selection = 0;
    } catch(std::exception const &error) { feedback = error.what(); }
    break;
  case interface_page::records:
  case interface_page::medals:
    page = interface_page::roster;
    selection = 0;
    break;
  case interface_page::boss:
  case interface_page::current_score:
    page = interface_page::flight;
    break;
  case interface_page::high_scores:
  case interface_page::sortie_summary:
    page = interface_page::menu;
    selection = 0;
    break;
  case interface_page::control_setup:
    if(selection < 3) adjust_controls();
    else if(selection == 3) {
      return_page = page;
      page = interface_page::help;
      help_index = 0;
    } else { page = interface_page::menu; selection = 0; }
    break;
  case interface_page::training:
    if(selection == 7) pending_launch = launch_request{
      .theatre{"training"},
      .training{training},
    };
    else if(selection == 8) { page = interface_page::menu; selection = 0; }
    else adjust_training(1);
    break;
  case interface_page::missions:
    if(selection == 0) { scroll = 0; page = interface_page::briefing; }
    else {
      if(selection - 1 < mission_starts.size()) pending_relocation = mission_starts[selection - 1];
      page = interface_page::flight;
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
    if(session_running) { page = interface_page::flight; break; }
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
        pending_finish = true;
      } else quit_requested = true;
    } else page = session_running ? interface_page::flight : interface_page::menu;
    selection = 0;
    break;
  case interface_page::outcome:
    page = latest_result ? interface_page::sortie_summary : interface_page::menu;
    session_running = false;
    selection = 0;
    break;
  case interface_page::flight:
    break;
  }
}
void front_end::command(interface_command const command) {
  /// Keyboard navigation operates on the current page, keeping flight input separate while a menu is open
  unsigned int const count{choice_count()};
  auto clamp_scroll{[&]{
    unsigned int lines{0};
    std::istringstream text{briefing};
    std::string line;
    while(std::getline(text, line)) if(line.find('\f') == std::string::npos && !line.starts_with("Sxyth")) ++lines;
    scroll = std::min(scroll, lines > 20 ? lines - 20 : 0);
  }};
  switch(command) {
  case interface_command::backspace:
    if(editing_name() && !entered_name.empty()) entered_name.pop_back();
    break;
  case interface_command::accept: accept(); break;
  case interface_command::up:
    if(page == interface_page::records || page == interface_page::medals) pilot_index = (pilot_index + static_cast<unsigned int>(campaign.pilots().size()) - 1) % static_cast<unsigned int>(campaign.pilots().size());
    else if(page == interface_page::briefing) { if(scroll != 0) --scroll; }
    else selection = (selection + count - 1) % count;
    break;
  case interface_command::down:
    if(page == interface_page::records || page == interface_page::medals) pilot_index = (pilot_index + 1) % static_cast<unsigned int>(campaign.pilots().size());
    else if(page == interface_page::briefing) { ++scroll; clamp_scroll(); }
    else selection = (selection + 1) % count;
    break;
  case interface_command::page_down:
    if(page == interface_page::records) records = static_cast<record_view>((std::to_underlying(records) + 1) % 4);
    else if(page == interface_page::briefing) { scroll += 17; clamp_scroll(); }
    break;
  case interface_command::page_up:
    if(page == interface_page::records) records = static_cast<record_view>((std::to_underlying(records) + 3) % 4);
    else scroll = scroll > 17 ? scroll - 17 : 0;
    break;
  case interface_command::previous:
    if(page == interface_page::medals) page = interface_page::records;
    else if(page == interface_page::records) page = interface_page::medals;
    if(page == interface_page::high_scores) score_list = (score_list + 2) % 3;
    if(page == interface_page::training) adjust_training(-1);
    if(page == interface_page::control_setup) adjust_controls();
    if(page == interface_page::help && help_index != 0) --help_index;
    break;
  case interface_command::next:
    if(page == interface_page::medals) page = interface_page::records;
    else if(page == interface_page::records) page = interface_page::medals;
    if(page == interface_page::high_scores) score_list = (score_list + 1) % 3;
    if(page == interface_page::training) adjust_training(1);
    if(page == interface_page::control_setup) adjust_controls();
    if(page == interface_page::help || page == interface_page::story || page == interface_page::title) accept();
    break;
  case interface_command::cancel:
    if(page == interface_page::radio) accept();
    else if(page == interface_page::ending) { ending.reset(); show_outcome(14); }
    else if(page == interface_page::flight) ask_to_end_sortie();
    else if(page == interface_page::help) page = return_page;
    else if(session_running && (page == interface_page::briefing || page == interface_page::missions || page == interface_page::current_score || page == interface_page::boss)) page = interface_page::flight;
    else if(page == interface_page::quit_confirmation) page = session_running ? interface_page::flight : interface_page::menu;
    else if(page == interface_page::pilots || page == interface_page::pilot_name || page == interface_page::pilot_delete || page == interface_page::records || page == interface_page::medals) { page = interface_page::roster; selection = 0; feedback.clear(); }
    else if(page == interface_page::title || page == interface_page::story) { title.reset(); page = interface_page::menu; }
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
void front_end::begin_ending(graphics::indexed_framebuffer const &backdrop, std::array<render::rgba_pixel, 16> const &colours) {
  /// Preserve the final sortie page while the single executable performs the original SRATS handoff
  ending.emplace(backdrop, colours);
  page = interface_page::ending;
}
void front_end::advance(double const seconds) {
  /// Helper animations continue on wall time while flight state remains stopped
  if(page == interface_page::story) {
    double constexpr palette_steps_per_second{40.0}, maximum_intensity{16.0};
    story_intensity = std::clamp(story_intensity + seconds * palette_steps_per_second * (advancing_story ? -1.0 : 1.0), 0.0, maximum_intensity);
    if(advancing_story && story_intensity == 0.0) {
      advancing_story = false;
      if(++story_index == stories.size()) { title.emplace(assets); page = interface_page::title; }
    }
  }
  if(page == interface_page::title) title->advance(seconds);
  if(page == interface_page::ending) {
    ending->advance(seconds);
    if(ending->finished()) { ending.reset(); show_outcome(14); }
  }
}
void front_end::show_radio(resources::radio_notice const notice, render::framebuffer const &backdrop, bool const enter_map) {
  /// BLACKBOX overlays the retained flight screen and pauses flight until the pilot acknowledges its original text
  radio = resources::read_radio_message(notice);
  radio_backdrop = backdrop;
  map_after_radio = enter_map;
  page = interface_page::radio;
}
void front_end::leave_sortie() {
  /// The flight session has already applied the original one-time Escape warning rule
  pending_finish = true;
}
void front_end::abort_sortie() {
  /// Control-C is a distinct original exit path with the abort result bit and no awarded score
  pending_abort = true;
  pending_finish = true;
}
bool front_end::take_abort() {
  /// Consume abort status alongside the single finalization request
  return std::exchange(pending_abort, false);
}
bool front_end::take_map() {
  /// Map instructions return to the original retained session before entering its independent observer loop
  return std::exchange(pending_map, false);
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
  if(!choices.empty()) canvas.text(80, 92, page == interface_page::training || page == interface_page::control_setup
    ? "Left/Right changes values; Enter selects" : "Use arrow keys and Enter to choose an action:", {85, 255, 255});
  unsigned int const first{selection < 8 ? 0 : selection - 7};
  for(unsigned int n{first}; n < choices.size() && n < first + 8; ++n) {
    int const y{112 + static_cast<int>(n - first) * 24};
    if(n == selection) canvas.rectangle(96, y, 600, y + 16, {0, 0, 85});
    canvas.text(112, y, choices[n], n == selection ? yellow : grey);
  }
  if(!feedback.empty()) canvas.text(24, 316, feedback.substr(0, 74), yellow);
  canvas.text(72, 348, "Arrows select   Enter accepts   Esc returns", grey);
  canvas.text(72, 368, "F4: flight controls and help", grey);
}
void front_end::render(render::framebuffer &target) const {
  /// Menus and helper-program artwork share the native window but retain their original video-mode pixel sizes
  if(page == interface_page::radio) {
    target = radio_backdrop;
    graphics::interface_canvas canvas{target, assets.file("font14.bin"), 14};
    auto fill{[&](resources::message_box const &box, render::rgba_pixel const colour){canvas.rectangle(box.left, box.top, box.left + box.width, box.top + box.height, colour);}};
    fill(radio->border, {170, 0, 0});
    fill(radio->background, {0, 0, 0});
    for(auto const &line : radio->lines) canvas.text(line.column * 8, line.row * 14, line.text, grey);
    return;
  }
  if(page == interface_page::ending) { ending->render(target); return; }
  if(page == interface_page::boss) {
    graphics::interface_canvas canvas{target, assets.file("font16.bin"), 16, 640, 400};
    canvas.clear();
    auto text{std::string{assets.text("boss.txt")}};
    if(auto const end{text.find('$')}; end != std::string::npos) text.resize(end);
    canvas.text(0, 0, text, grey);
    return;
  }
  if(page == interface_page::story) {
    graphics::draw_presentation(target, assets, stories.at(story_index));
    unsigned int const intensity{static_cast<unsigned int>(story_intensity)};
    if(intensity < 16) {
      auto channel{[intensity](uint8_t const value){
        unsigned int const dac{(value >> 2) * intensity / 16};
        return static_cast<uint8_t>((dac << 2) | (dac >> 4));
      }};
      for(auto &pixel : target.pixels) pixel = {
        .red{channel(pixel.red)},
        .green{channel(pixel.green)},
        .blue{channel(pixel.blue)},
      };
    }
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
    title->render(target);
    return;
  }
  bool const intelligence{page == interface_page::briefing};
  graphics::interface_canvas canvas{target, intelligence ? assets.file("font14.bin") : std::span<uint8_t const>{menu_font}, intelligence ? 14u : 16u, 640, intelligence ? 350u : 400u};
  if(page == interface_page::menu) {
    static std::array<std::string_view, 8> constexpr choices{"Duty roster", "Combat missions", "Training missions", "Pilot records", "High-score tables", "Controls and sound setup", "Introduction", "Quit"};
    static std::array<std::string_view, 3> constexpr resume{"Resume flight", "Flight controls and help", "End current sortie"};
    draw_menu(canvas, "Main menu", session_running ? std::span<std::string_view const>{resume} : std::span<std::string_view const>{choices});
    auto const *pilot{campaign.active_pilot()};
    canvas.text(72, 304, pilot ? pilot->name() + " - " + std::string{rank_name(pilot->rank())} : "Trainee - progress is not saved", grey);
  } else if(page == interface_page::control_setup) {
    std::string const device{controls.joystick_device == -2 ? "keyboard only" : controls.joystick_device == -1 ? "automatic joystick" : "joystick " + std::to_string(controls.joystick_device)};
    std::array<std::string, 5> const labels{
      "Controls: " + device,
      std::string{"Flight model: "} + (controls.beginner_flight ? "beginner" : "advanced"),
      std::string{"Music: "} + (music_on ? "on" : "off"),
      "Flight controls and help", "Return to main menu",
    };
    std::array<std::string_view, 5> choices;
    std::ranges::copy(labels, choices.begin());
    draw_menu(canvas, "Controls and sound setup", choices);
    canvas.text(64, 240, "Beginner: gentle pitch assistance toward level flight", grey);
    canvas.text(64, 264, "Joystick buttons 1/2: guns/missiles", grey);
    canvas.text(64, 288, "Keyboard arrows override the stick during flight", grey);
  } else if(page == interface_page::training) {
    auto quality{[](int const q){return q < 0 ? std::string{"random"} : std::to_string(q + 1);}};
    std::array<std::string, 9> const labels{
      "KLA sites: " + std::to_string(training.kla.count), "KLA quality: " + quality(training.kla.quality),
      "Vehicle sites: " + std::to_string(training.vehicles.count), "Vehicle quality: " + quality(training.vehicles.quality),
      "AAA sites: " + std::to_string(training.aaa.count), "AAA quality: " + quality(training.aaa.quality),
      std::string{"Invulnerability: "} + (training.invincible ? "on" : "off"), "Start training flight", "Return to main menu",
    };
    std::array<std::string_view, 9> choices;
    std::ranges::copy(labels, choices.begin());
    draw_menu(canvas, "Training missions", choices);
  } else if(page == interface_page::missions) {
    std::vector<std::string_view> choices{"Read full airbase intelligence"};
    for(auto const &start : mission_starts) choices.push_back(start.title);
    draw_menu(canvas, "Intelligence - choose an airborne start", choices);
    if(choices.empty()) canvas.text(80, 160, "No airborne starts in this theatre", grey);
  } else if(page == interface_page::theatres) draw_menu(canvas, "Choose a theatre of operations", theatre_titles);
  else if(page == interface_page::roster) {
    static std::array<std::string_view, 8> constexpr choices{"Create pilot", "Activate pilot", "View pilot records", "Rename pilot", "Resurrect pilot", "Deactivate pilot (use trainee)", "Delete pilot", "Return to main menu"};
    draw_menu(canvas, "Duty roster", choices);
  } else if(page == interface_page::pilots) {
    std::vector<std::string> names;
    for(auto const &pilot : campaign.pilots()) names.push_back(pilot.name() + (pilot.unavailable() ? " (killed or captured)" : pilot.selected() ? " (active)" : ""));
    std::vector<std::string_view> choices{names.begin(), names.end()};
    draw_menu(canvas, "Choose a pilot", choices);
  } else if(page == interface_page::pilot_delete) {
    static std::array<std::string_view, 2> constexpr choices{"Yes - delete pilot and saved theatres", "No - return to roster"};
    draw_menu(canvas, "Delete " + campaign.pilots()[pilot_index].name() + "?", choices);
  } else if(page == interface_page::pilot_name) {
    draw_menu(canvas, renaming ? "Rename pilot" : "Create pilot", {});
    canvas.text(80, 140, "Pilot name (1 to 20 characters):", grey);
    canvas.rectangle(80, 176, 560, 200, {0, 0, 85});
    canvas.text(88, 180, entered_name + "_", yellow);
  } else if(page == interface_page::records || page == interface_page::sortie_summary || page == interface_page::current_score) {
    draw_menu(canvas, page == interface_page::records ? "Pilot records" : "Sortie report", {});
    pilot_record const *pilot{page == interface_page::records ? &campaign.pilots()[pilot_index] : campaign.active_pilot()};
    bool const record_page{page == interface_page::records};
    auto const result{!record_page ? *latest_result : records == record_view::latest ? pilot->last_sortie()
      : records == record_view::best ? pilot->best_sortie() : records == record_view::individual ? pilot->individual_records() : pilot->career()};
    if(record_page) {
      static std::array<std::string_view, 4> constexpr labels{"Career totals", "Latest sortie", "Best sortie", "Individual records"};
      canvas.text(72, 88, labels[std::to_underlying(records)], yellow);
    }
    canvas.text(72, 120, pilot ? pilot->name() + " - " + std::string{rank_name(pilot->rank())} : "Trainee", yellow);
    int32_t const score{!record_page ? calculate_score(result) : records == record_view::latest ? pilot->last_sortie_score()
      : records == record_view::best ? pilot->best_sortie_score() : pilot->career_score()};
    if(!record_page || records != record_view::individual) canvas.text(72, 152, "Score: " + std::to_string(score), grey);
    canvas.text(72, 176, "Objectives completed: " + std::to_string(result.objectives_completed), grey);
    canvas.text(72, 200, "Aircraft lost: " + std::to_string(result.aircraft_lost), grey);
    canvas.text(72, 224, "Friendly losses: " + std::to_string(result.friendly_destroyed), grey);
    canvas.text(72, 248, std::string{record_page && records == record_view::career ? "Accumulated damage: " : "Maximum damage: "} + std::to_string(result.maximum_damage), grey);
    auto const seconds{result.elapsed_seconds};
    canvas.text(72, 272, std::format("Flying time: {}:{:02}:{:02}", seconds / 3600, seconds / 60 % 60, seconds % 60)
      + (pilot ? "   Career sorties: " + std::to_string(pilot->sorties()) : ""), grey);
    canvas.text(72, 304, page == interface_page::records ? "PgUp/PgDn: records  Left/Right: medals  Enter: roster" : "Press Enter to continue", yellow);
  } else if(page == interface_page::medals) {
    draw_menu(canvas, "Pilot medals", {});
    auto const &pilot{campaign.pilots()[pilot_index]};
    canvas.text(72, 88, pilot.name() + " - " + std::string{rank_name(pilot.rank())}, yellow);
    for(unsigned int i{0}; i != medal_definitions.size(); ++i) {
      auto const &award{medal_definitions[i]};
      unsigned int const count{pilot.awards(award.kind)};
      int const row{116 + static_cast<int>(i) * 18};
      auto const colour{count == 0 ? render::ega_palette[8] : render::ega_palette[award.colour]};
      canvas.text(80, row, std::string_view{award.glyphs.data(), award.glyphs.size()}, colour);
      canvas.text(112, row, award.name, count == 0 ? grey : white);
      canvas.text(496, row, std::to_string(count), count == 0 ? grey : yellow);
    }
    canvas.text(72, 316, "Left/Right: records  Up/Down: pilot  Enter: roster", yellow);
  } else if(page == interface_page::high_scores) {
    std::array<std::string_view, 3> constexpr titles{"Best missions", "Top careers", "Fastest careers"};
    draw_menu(canvas, titles[score_list], {});
    canvas.text(32, 92, score_list == 0 ? "    Pilot                Theatre                     Points" : "    Pilot                  Sorties  Revivals       Points", grey);
    auto const &tables{campaign.high_scores()};
    for(unsigned int i{0}; i != 10; ++i) {
      std::string row;
      if(score_list == 0) {
        auto const &entry{tables.best_missions()[i]};
        row = std::format("{:2}  {:20} {:22} {:>9}", i + 1, (entry.retired ? "*" : "") + score_text(entry.name), score_text(entry.theatre), entry.points);
      } else {
        auto const &entry{(score_list == 1 ? tables.best_careers() : tables.fastest_careers())[i]};
        auto const points{score_list == 1 || entry.sorties == 0 ? entry.points : entry.points / entry.sorties};
        row = std::format("{:2}  {:20} {:>7} {:>9} {:>13}", i + 1, (entry.retired ? "*" : "") + score_text(entry.name), entry.sorties, entry.resurrections, points);
      }
      canvas.text(32, 120 + static_cast<int>(i) * 18, row, i == 0 ? yellow : grey);
    }
    canvas.text(40, 316, score_list == 2 ? "Points per sortie; at least three sorties required" : "Left/Right: score tables   * retired pilot", yellow);
  } else if(page == interface_page::quit_confirmation) {
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
    canvas.text(8, 326, session_running ? "Up/Down or PgUp/PgDn scroll   Enter/Esc returns to flight" : "Up/Down or PgUp/PgDn scroll   Enter starts   Esc returns", yellow);
  }
}
unsigned int front_end::choice_count() const {
  /// Only selection pages wrap; text entry and reports keep a neutral selection
  switch(page) {
  case interface_page::menu: return session_running ? 3 : 8;
  case interface_page::roster: return 8;
  case interface_page::pilot_delete: return 2;
  case interface_page::theatres: return 4;
  case interface_page::training: return 9;
  case interface_page::control_setup: return 5;
  case interface_page::missions: return static_cast<unsigned int>(mission_starts.size()) + 1;
  case interface_page::pilots: return std::max(1u, static_cast<unsigned int>(campaign.pilots().size()));
  case interface_page::quit_confirmation: return 2;
  default: return 1;
  }
}
void front_end::adjust_controls() {
  /// The original front end selects keyboard/joystick control and beginner/advanced aerodynamics before a flight
  if(selection == 0) {
    controls.joystick_device = controls.joystick_device == -2 ? -1 : -2;
    campaign.keyboard_control(controls.joystick_device == -2);
  } else if(selection == 1) {
    controls.beginner_flight = !controls.beginner_flight;
    campaign.beginner_flight(controls.beginner_flight);
  } else if(selection == 2) {
    music_on = !music_on;
    campaign.music_enabled(music_on);
  }
}
audio::music_cue front_end::music() const noexcept {
  /// The DOS launcher plays music; flight, its helper pages and the original outcome/ending sequences do not
  if(!music_on || session_running || page == interface_page::ending || page == interface_page::outcome || page == interface_page::story)
    return audio::music_cue::silent;
  return page == interface_page::title ? audio::music_cue::title : audio::music_cue::menus;
}
control_setup const &front_end::flight_controls() const noexcept {
  /// New sessions and platform polling consume the same settings shown by the front end
  return controls;
}
void front_end::adjust_training(int const direction) {
  /// Training configures original site counts and qualities; no extra difficulty rules are invented
  if(selection == 6) training.invincible = !training.invincible;
  else if(selection < 6) {
    auto &defence{selection < 2 ? training.kla : selection < 4 ? training.vehicles : training.aaa};
    if((selection & 1u) == 0) defence.count = static_cast<unsigned int>(std::clamp(static_cast<int>(defence.count) + direction, 0, static_cast<int>(world::training_defence::maximum_menu_count)));
    else defence.quality = std::clamp(defence.quality + direction, -1, 7);
  } else return;
  campaign.training_settings(training);
}
void front_end::show_intelligence(world::theatre const &world) {
  /// The retained session supplies its current theatre's complete intelligence text
  session_running = true;
  briefing = world.briefing;
  scroll = 0;
  page = interface_page::briefing;
}
void front_end::show_missions(world::theatre const &world) {
  /// Intelligence starts are explicit directives in the original theatre text
  session_running = true;
  mission_starts = world.missions;
  briefing = world.briefing;
  selection = 0;
  page = interface_page::missions;
}
void front_end::show_boss() {
  /// F1 retains the original deliberately uninteresting DOS prompt disguise
  session_running = true;
  page = interface_page::boss;
}
void front_end::show_current_score(sortie_statistics const &result) {
  /// F3 inspects this sortie without finalizing or saving it
  latest_result = result;
  session_running = true;
  page = interface_page::current_score;
}
std::optional<world::mission_start> front_end::take_relocation() {
  /// Consume a mission selection without recreating the theatre or losing already destroyed objectives
  return std::exchange(pending_relocation, std::nullopt);
}
void front_end::character(uint32_t const codepoint) {
  /// GLFW text callbacks preserve spaces without treating them as menu acceptance
  if(editing_name() && codepoint >= 32 && codepoint < 127 && entered_name.size() < 20) entered_name += static_cast<char>(codepoint);
}
bool front_end::editing_name() const noexcept {
  /// Text input owns printable characters only during a pilot-name prompt
  return page == interface_page::pilot_name;
}
bool front_end::take_finish() {
  /// Consume one confirmed end request; the application records and presents the actual result
  return std::exchange(pending_finish, false);
}
void front_end::finish_sortie(sortie_statistics const &result) {
  /// Match native result priority before showing the sortie statistics in the same executable
  latest_result = result;
  session_running = false;
  unsigned int panel{9};
  if(result.has(sortie_status::earth_saved)) panel = 14;
  else if(result.has(sortie_status::killed)) {
    panel = result.has(sortie_status::ejected) ? 7 : 4;
    if(result.has(sortie_status::ejected)) switch(static_cast<world::object_type>(result.contacting_type)) {
    case world::object_type::aa_shell: case world::object_type::shell: panel = 10; break;
    case world::object_type::kla: panel = 11; break;
    case world::object_type::crater: panel = 12; break;
    case world::object_type::bee_swarm: case world::object_type::barrage_balloon: panel = 13; break;
    default: break;
    }
  } else if(result.has(sortie_status::captured)) panel = 8;
  else if(result.has(sortie_status::home) && result.friendly_destroyed != 0) panel = 6;
  else if(result.has(sortie_status::rescued)) panel = 1;
  else if(result.has(sortie_status::crashed_away)) panel = 2;
  else if(result.has(sortie_status::crashed_home)) panel = 3;
  else if(result.has(sortie_status::home)) panel = result.has(sortie_status::landed) ? 0 : 5;
  show_outcome(panel);
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

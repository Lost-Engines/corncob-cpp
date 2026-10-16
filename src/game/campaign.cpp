#include "campaign.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>
#include "platform/storage.h"

namespace corncob::game {

namespace {

unsigned int constexpr theatre_header_bytes{671}, embedded_pilot_offset{48};
std::string definition_name(std::string_view const theatre) {
  /// Native THT headers store a definition stem, without its CCT extension
  if(theatre == "morning" || theatre == "dusk") return std::string{theatre};
  if(theatre == "white_sands") return "whtsands";
  if(theatre == "wasteland") return "wastelnd";
  throw std::invalid_argument{"unknown saved theatre"};
}
std::string_view theatre_title(std::string_view const theatre) {
  /// Retain the original theatre titles in DOS-compatible saved headers
  if(theatre == "morning") return "Atkinsville (easy)";
  if(theatre == "dusk") return "Dusk (medium)";
  if(theatre == "wasteland") return "Wasteland (medium)";
  if(theatre == "white_sands") return "White Sands Testing Ground";
  throw std::invalid_argument{"unknown saved theatre"};
}
std::string stored_text(std::span<uint8_t const> const field) {
  /// Fixed native fields are bounded even when imported padding is not terminated
  auto const end{std::ranges::find(field, 0)};
  return {reinterpret_cast<char const*>(field.data()), static_cast<size_t>(end - field.begin())};
}
std::string stored_definition(std::span<uint8_t const> const bytes) {
  /// Accept the earlier reconstruction's uppercase CCT suffix as well as original DOS stems
  auto name{stored_text(bytes.subspan(28, 12))};
  std::ranges::transform(name, name.begin(), [](unsigned char const c){return static_cast<char>(std::tolower(c));});
  if(name.ends_with(".cct")) name.resize(name.size() - 4);
  return name;
}
std::vector<std::filesystem::path> legacy_theatres(std::filesystem::path const &directory) {
  /// Earlier native builds placed slot-named snapshots beside the roster; keep their saved progress readable
  std::vector<std::filesystem::path> result;
  if(!std::filesystem::exists(directory)) return result;
  for(auto const &entry : std::filesystem::directory_iterator{directory}) {
    if(!entry.is_regular_file() || !entry.path().filename().string().starts_with("pilot_")) continue;
    auto extension{entry.path().extension().string()};
    std::ranges::transform(extension, extension.begin(), [](unsigned char const c){return static_cast<char>(std::tolower(c));});
    if(extension == ".tht") result.push_back(entry.path());
  }
  std::ranges::sort(result);
  return result;
}
std::vector<std::filesystem::path> owned_theatres(std::filesystem::path const &directory, pilot_record const &pilot) {
  /// Original records link numbered files explicitly; legacy slot-named snapshots predate those references
  std::vector<std::filesystem::path> result;
  for(auto const number : pilot.theatres()) {
    auto const path{directory / "THT" / (std::to_string(number) + ".THT")};
    if(std::filesystem::exists(path)) result.push_back(path);
  }
  for(auto const &path : legacy_theatres(directory)) {
    auto const bytes{platform::read_binary(path)};
    if(bytes.size() >= theatre_header_bytes && stored_text(std::span{bytes}.subspan(embedded_pilot_offset, 22)) == pilot.name()) result.push_back(path);
  }
  return result;
}

bool required_theatre(std::string_view const name) {
  /// The normal Earth campaign requires these three combat theatres; White Sands is its subsequent test flight
  return name == "morning" || name == "dusk" || name == "wasteland";
}
unsigned int remaining_objectives(world::theatre const &theatre) {
  /// INT's survey treats preset-complete anchors separately from objectives still available to the pilot
  return static_cast<unsigned int>(std::ranges::count_if(theatre.objects, [](auto const &object){
    if(!object.has(world::object_flag::present) || object.has(world::object_flag::objective_complete)) return false;
    using enum world::object_type;
    return object.type == generator_orb || object.type == destroyed_generator_orb || object.type == objective_saucer
      || (object.type == ground_crew && object.has(world::object_flag::special))
      || (object.type == portal && (object.quality() == 4 || object.quality() == 7));
  }));
}

} // anonymous namespace

campaign_store::campaign_store(std::filesystem::path directory) :
  directory{std::move(directory)} {
  /// Load only user save state; original scenery remains embedded in the executable
  auto const path{this->directory / "PILOT.SCR"};
  if(std::filesystem::exists(path)) roster = pilot_roster::decode(platform::read_binary(path));
  for(unsigned int i{0}; i != roster.pilots.size(); ++i) {
    if(roster.pilots[i].selected() && !roster.pilots[i].unavailable()) {
      selected = i;
      break;
    }
  }
  auto const high_scores{this->directory / "PILOT.HSC"};
  if(std::filesystem::exists(high_scores)) scores = score_tables::decode(platform::read_binary(high_scores));
}
std::filesystem::path campaign_store::theatre_path(std::string_view const theatre) const {
  /// Recover the original ownership arrangement and allocate a free numbered path only for a new theatre
  if(!selected) throw std::logic_error{"training pilot has no persistent theatre"};
  for(auto const &path : owned_theatres(directory, roster.pilots[*selected])) {
    auto const bytes{platform::read_binary(path)};
    if(stored_definition(bytes) == definition_name(theatre)) return path;
  }
  for(unsigned int index{1}; index <= 255; ++index) {
    auto const path{directory / "THT" / (std::to_string(index) + ".THT")};
    if(!std::filesystem::exists(path)) return path;
  }
  throw std::runtime_error{"no available saved-theatre filename"};
}
void campaign_store::save_roster() const {
  /// Write the original masked roster representation after a deliberate roster or sortie change
  platform::write_binary(directory / "PILOT.SCR", roster.encode());
}
std::span<pilot_record const> campaign_store::pilots() const noexcept {
  /// The interface can inspect the roster without altering opaque original record fields
  return roster.pilots;
}
pilot_record const *campaign_store::active_pilot() const noexcept {
  /// No named active pilot means the original disposable trainee role
  return selected ? &roster.pilots[*selected] : nullptr;
}
unsigned int campaign_store::create_pilot(std::string_view const name) {
  /// Pilot creation validates the bounded native name before changing saved state
  if(std::ranges::any_of(roster.pilots, [name](pilot_record const &pilot){return pilot.name() == name;}))
    throw std::invalid_argument{"a pilot with that name already exists"};
  roster.pilots.emplace_back(name);
  unsigned int const index{static_cast<unsigned int>(roster.pilots.size() - 1)};
  activate(index);
  return index;
}
void campaign_store::activate(unsigned int const index) {
  /// Activating one living pilot clears the selection marker from all other records
  if(index >= roster.pilots.size()) throw std::invalid_argument{"pilot selection is outside the roster"};
  if(roster.pilots[index].unavailable()) throw std::invalid_argument{"that pilot is killed or captured; resurrect the pilot first"};
  for(unsigned int i{0}; i != roster.pilots.size(); ++i) roster.pilots[i].select(i == index);
  selected = index;
  save_roster();
}
void campaign_store::deactivate() {
  /// Deactivating returns to the disposable trainee without deleting saved careers
  for(auto &pilot : roster.pilots) pilot.select(false);
  selected.reset();
  save_roster();
}
void campaign_store::rename_pilot(unsigned int const index, std::string_view const name) {
  /// Rename all owned theatre headers and live high-score entries before replacing the roster name
  if(index >= roster.pilots.size()) throw std::invalid_argument{"pilot selection is outside the roster"};
  pilot_record renamed{roster.pilots[index]};
  renamed.rename(name);
  for(unsigned int i{0}; i != roster.pilots.size(); ++i)
    if(i != index && roster.pilots[i].name() == name) throw std::invalid_argument{"a pilot with that name already exists"};
  auto const previous{roster.pilots[index].name()};
  for(auto const &path : owned_theatres(directory, roster.pilots[index])) {
    auto bytes{platform::read_binary(path)};
    std::ranges::copy(renamed.native_data().first(22), bytes.begin() + embedded_pilot_offset);
    platform::write_binary(path, bytes);
  }
  scores.rename(previous, name);
  platform::write_binary(directory / "PILOT.HSC", scores.encode());
  roster.pilots[index] = renamed;
  save_roster();
}
void campaign_store::resurrect(unsigned int const index) {
  /// Preserve the original resurrection counter while making the selected career available again
  if(index >= roster.pilots.size()) throw std::invalid_argument{"pilot selection is outside the roster"};
  roster.pilots[index].resurrect();
  save_roster();
}
void campaign_store::delete_pilot(unsigned int const index) {
  /// Deleting a career removes its owned worlds and retires its achievements; other pilots retain their file identities
  if(index >= roster.pilots.size()) throw std::invalid_argument{"pilot selection is outside the roster"};
  auto const name{roster.pilots[index].name()};
  for(auto const &path : owned_theatres(directory, roster.pilots[index])) std::filesystem::remove(path);
  scores.retire(name);
  platform::write_binary(directory / "PILOT.HSC", scores.encode());
  roster.pilots.erase(roster.pilots.begin() + index);
  if(selected && *selected == index) selected.reset();
  else if(selected && *selected > index) --*selected;
  save_roster();
}
bool campaign_store::keyboard_control() const noexcept {
  /// The original global control choice is independent of the active career
  return roster.keyboard_control();
}
void campaign_store::keyboard_control(bool const value) {
  /// Persist the digital control choice in its native header byte
  roster.keyboard_control(value);
  save_roster();
}
void campaign_store::beginner_flight(bool const value) {
  /// A named career retains its original aerodynamic-model bit; the disposable trainee has no saved pilot record
  if(!selected) return;
  roster.pilots[*selected].beginner_flight(value);
  save_roster();
}
std::optional<world::theatre> campaign_store::load_world(std::string_view const theatre) const {
  /// THT keeps the original 671-byte header and compact live world rather than resetting destroyed scenery each sortie
  if(!selected) return std::nullopt;
  auto const path{theatre_path(theatre)};
  if(!std::filesystem::exists(path)) return std::nullopt;
  auto const data{platform::read_binary(path)};
  if(data.size() < theatre_header_bytes) throw std::invalid_argument{"saved theatre header is truncated: " + path.string()};
  std::string const expected{definition_name(theatre)};
  if(stored_definition(data) != expected) throw std::invalid_argument{"saved theatre refers to a different definition: " + path.string()};
  return world::decode_theatre(std::span{data}.subspan(theatre_header_bytes));
}
void campaign_store::finish_sortie(std::string_view const theatre, world::theatre const &world, sortie_statistics const &result) {
  /// A trainee does not alter a named career; named pilots retain world destruction and their latest native result block
  if(!selected) { save_scores(theatre, result); return; }
  auto &pilot{roster.pilots[*selected]};
  pilot.finish_sortie(result);
  if(result.counts_for_career() && required_theatre(theatre) && remaining_objectives(world) == 0) {
    auto const previous{load_world(theatre)};
    if(!previous || remaining_objectives(*previous) != 0) {
      pilot.award(medal::theatre_combat);
      if(!result.has(sortie_status::invincible)) pilot.award(medal::distinguished_flying_cross);
    }
    bool all_completed{true};
    for(auto const name : {"morning", "dusk", "wasteland"}) {
      if(name == theatre) continue;
      auto const saved{load_world(name)};
      if(!saved || remaining_objectives(*saved) != 0) all_completed = false;
    }
    if(all_completed && pilot.awards(medal::combat_master) == 0 && !result.has(sortie_status::killed)) pilot.award(medal::combat_master);
  }
  if(theatre == "training") { save_roster(); save_scores(theatre, result); return; }
  auto const path{theatre_path(theatre)};
  if(path.parent_path() == directory / "THT") pilot.add_theatre(static_cast<uint8_t>(std::stoul(path.stem().string())));
  std::vector<uint8_t> saved(theatre_header_bytes);
  if(std::filesystem::exists(path)) {
    auto const previous{platform::read_binary(path)};
    std::copy_n(previous.begin(), theatre_header_bytes, saved.begin());
  }
  auto const title{theatre_title(theatre)};
  std::fill_n(saved.begin(), 40, 0);
  std::ranges::copy(title.substr(0, 27), saved.begin());
  auto const definition{definition_name(theatre)};
  std::ranges::copy(definition, saved.begin() + 28);
  auto const record{pilot.native_data()};
  std::ranges::copy(record, saved.begin() + embedded_pilot_offset);
  auto const objects{world::encode_theatre(world)};
  saved.insert(saved.end(), objects.begin(), objects.end());
  platform::write_binary(path, saved);
  save_roster();
  save_scores(theatre, result);
}
void campaign_store::save_scores(std::string_view const theatre, sortie_statistics const &result) {
  /// Persist all three native score lists independently of the selected pilot or live world
  scores.record(theatre, result, active_pilot());
  platform::write_binary(directory / "PILOT.HSC", scores.encode());
}
score_tables const &campaign_store::high_scores() const noexcept {
  /// Reports read immutable ranked tables from the same save store as the roster
  return scores;
}
std::filesystem::path const &campaign_store::save_directory() const noexcept {
  /// Report the canonical user save location for help and diagnostics
  return directory;
}

} // namespace corncob::game

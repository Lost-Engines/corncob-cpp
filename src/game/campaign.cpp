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
  /// Saved-theatre headers refer back to the original definition basename
  if(theatre == "white_sands") return "WHTSANDS.CCT";
  if(theatre == "wasteland") return "WASTELND.CCT";
  if(theatre == "dusk") return "DUSK.CCT";
  if(theatre == "morning") return "MORNING.CCT";
  throw std::invalid_argument{"unknown saved theatre"};
}

} // anonymous namespace

campaign_store::campaign_store(std::filesystem::path directory) :
  directory{std::move(directory)} {
  /// Load only user save state; original scenery remains embedded in the executable
  auto const path{this->directory / "PILOT.SCR"};
  if(!std::filesystem::exists(path)) return;
  roster = pilot_roster::decode(platform::read_binary(path));
  for(unsigned int i{0}; i != roster.pilots.size(); ++i) {
    if(roster.pilots[i].selected() && !roster.pilots[i].unavailable()) {
      selected = i;
      break;
    }
  }
}
std::filesystem::path campaign_store::theatre_path(std::string_view const theatre) const {
  /// Keep independent live worlds for each named pilot and theatre without putting typed filenames into pilot records
  if(!selected) throw std::logic_error{"training pilot has no persistent theatre"};
  return directory / ("pilot_" + std::to_string(*selected + 1) + "_" + std::string{theatre} + ".THT");
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
  /// Stable roster-slot snapshot names let a pilot be renamed without moving or duplicating saved worlds
  if(index >= roster.pilots.size()) throw std::invalid_argument{"pilot selection is outside the roster"};
  roster.pilots[index].rename(name);
  save_roster();
}
void campaign_store::resurrect(unsigned int const index) {
  /// Preserve the original resurrection counter while making the selected career available again
  if(index >= roster.pilots.size()) throw std::invalid_argument{"pilot selection is outside the roster"};
  roster.pilots[index].resurrect();
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
  std::string_view const stored{reinterpret_cast<char const*>(data.data() + 28), 12};
  if(stored.substr(0, stored.find('\0')) != expected) throw std::invalid_argument{"saved theatre refers to a different definition: " + path.string()};
  return world::decode_theatre(std::span{data}.subspan(theatre_header_bytes));
}
void campaign_store::finish_sortie(std::string_view const theatre, world::theatre const &world, sortie_statistics const &result) {
  /// A trainee does not alter a named career; named pilots retain world destruction and their latest native result block
  if(!selected) return;
  auto &pilot{roster.pilots[*selected]};
  pilot.finish_sortie(result);
  std::vector<uint8_t> saved(theatre_header_bytes);
  auto const title{std::string{theatre}};
  std::ranges::copy(title.substr(0, 27), saved.begin());
  auto const definition{definition_name(theatre)};
  std::ranges::copy(definition, saved.begin() + 28);
  auto const record{pilot.native_data()};
  std::ranges::copy(record, saved.begin() + embedded_pilot_offset);
  auto const objects{world::encode_theatre(world)};
  saved.insert(saved.end(), objects.begin(), objects.end());
  platform::write_binary(theatre_path(theatre), saved);
  save_roster();
}
std::filesystem::path const &campaign_store::save_directory() const noexcept {
  /// Report the canonical user save location for help and diagnostics
  return directory;
}

} // namespace corncob::game

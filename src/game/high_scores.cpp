#include "high_scores.h"
#include <algorithm>
#include <stdexcept>
#include "resources/binary_reader.h"

namespace corncob::game {

namespace {

score_name name_bytes(std::string_view const text) {
  /// New entries are terminated; imported fixed-width fields retain all their original bytes
  score_name result{};
  std::copy_n(text.begin(), std::min(text.size(), result.size() - 1), result.begin());
  return result;
}
int32_t points_per_sortie(career_score const &entry) {
  /// Fastest-career ranking uses integer points per mission after at least three missions
  return entry.sorties == 0 ? 0 : entry.points / entry.sorties;
}
void update_career(std::array<career_score, 10> &table, career_score const &candidate, bool const fastest) {
  /// Each career occupies one entry; subsequent sorties replace its totals before ranking it again
  std::vector<career_score> entries;
  for(auto const &entry : table) if(entry.retired != 0 || score_text(entry.name) != score_text(candidate.name)) entries.push_back(entry);
  entries.push_back(candidate);
  std::ranges::stable_sort(entries, [fastest](auto const &a, auto const &b){
    return fastest ? points_per_sortie(a) > points_per_sortie(b) : a.points > b.points;
  });
  for(unsigned int i{0}; i != table.size(); ++i) table[i] = i < entries.size() ? entries[i] : career_score{};
}

} // anonymous namespace

std::string score_text(score_name const &value) {
  /// Display up to the first NUL without discarding opaque padding from saved entries
  auto const end{std::ranges::find(value, 0)};
  return {reinterpret_cast<char const*>(value.data()), static_cast<size_t>(end - value.begin())};
}
score_tables score_tables::decode(std::span<uint8_t const> const source) {
  /// PILOT.HSC has ten 180-byte mission records followed by two sets of ten 162-byte career records
  if(source.size() != native_bytes) throw std::invalid_argument{"invalid native high-score table length"};
  resources::binary_reader const data{source};
  score_tables result;
  unsigned int offset{0};
  auto name{[&](score_name &field){std::copy_n(source.begin() + offset, field.size(), field.begin()); offset += static_cast<unsigned int>(field.size());}};
  auto statistics{[&](sortie_statistics &field){field = sortie_statistics::decode(source.subspan(offset, sortie_statistics::native_bytes)); offset += sortie_statistics::native_bytes;}};
  for(auto &entry : result.missions) {
    name(entry.name);
    name(entry.theatre);
    statistics(entry.statistics);
    entry.points = data.signed_dword(offset);
    entry.retired = data.word(offset + 4);
    offset += 6;
  }
  for(auto *table : {&result.careers, &result.fastest}) for(auto &entry : *table) {
    name(entry.name);
    entry.sorties = data.word(offset);
    entry.resurrections = data.word(offset + 2);
    offset += 4;
    statistics(entry.statistics);
    entry.points = data.signed_dword(offset);
    entry.retired = data.word(offset + 4);
    offset += 6;
  }
  return result;
}
std::array<uint8_t, score_tables::native_bytes> score_tables::encode() const {
  /// Encode the exact original record sizes and preserve every fixed-width name byte and result field
  std::array<uint8_t, native_bytes> result{};
  unsigned int offset{0};
  auto word{[&](uint16_t const value){result[offset++] = static_cast<uint8_t>(value); result[offset++] = static_cast<uint8_t>(value >> 8);}};
  auto tail{[&](int32_t const points, uint16_t const retired){word(static_cast<uint16_t>(points)); word(static_cast<uint16_t>(static_cast<uint32_t>(points) >> 16)); word(retired);}};
  auto name{[&](score_name const &value){std::ranges::copy(value, result.begin() + offset); offset += static_cast<unsigned int>(value.size());}};
  auto statistics{[&](sortie_statistics const &value){auto const raw{value.encode()}; std::ranges::copy(raw, result.begin() + offset); offset += static_cast<unsigned int>(raw.size());}};
  for(auto const &entry : missions) {
    name(entry.name);
    name(entry.theatre);
    statistics(entry.statistics);
    tail(entry.points, entry.retired);
  }
  for(auto const *table : {&careers, &fastest}) for(auto const &entry : *table) {
    name(entry.name);
    word(entry.sorties);
    word(entry.resurrections);
    statistics(entry.statistics);
    tail(entry.points, entry.retired);
  }
  return result;
}
void score_tables::rename(std::string_view const previous, std::string_view const name) {
  /// Live achievements follow a renamed career; retired careers with a reused name remain distinct
  auto replace{[name](score_name &field){
    unsigned int const length{static_cast<unsigned int>(std::min(name.size(), field.size() - 1))};
    std::copy_n(name.begin(), length, field.begin());
    field[length] = 0;
  }};
  for(auto &entry : missions) if(entry.retired == 0 && score_text(entry.name) == previous) replace(entry.name);
  for(auto *table : {&careers, &fastest}) for(auto &entry : *table)
    if(entry.retired == 0 && score_text(entry.name) == previous) replace(entry.name);
}
void score_tables::retire(std::string_view const name) {
  /// Preserve a deleted pilot's historical results while allowing a later pilot to reuse the same name
  for(auto &entry : missions) if(score_text(entry.name) == name) entry.retired = 1;
  for(auto *table : {&careers, &fastest}) for(auto &entry : *table) if(score_text(entry.name) == name) entry.retired = 1;
}
void score_tables::record(std::string_view const theatre, sortie_statistics const &result, pilot_record const *const pilot) {
  /// Mission records are separate achievements; cumulative and fastest lists retain one current row per named career
  if(!result.counts_for_career()) return;
  auto const name{name_bytes(pilot ? pilot->name() : "trainee")};
  int32_t const points{calculate_score(result, score_context::campaign)};
  if(points > missions.back().points) {
    std::vector<mission_score> entries{missions.begin(), missions.end()};
    entries.push_back({
      .name{name},
      .theatre{name_bytes(theatre)},
      .statistics{result},
      .points{points},
    });
    std::ranges::stable_sort(entries, std::greater<>{}, &mission_score::points);
    std::copy_n(entries.begin(), missions.size(), missions.begin());
  }
  if(!pilot) return;
  career_score const career{
    .name{name},
    .sorties{pilot->sorties()},
    .resurrections{pilot->resurrections()},
    .statistics{pilot->career()},
    .points{pilot->career_score()},
  };
  update_career(careers, career, false);
  if(career.sorties >= 3) update_career(fastest, career, true);
}
std::span<mission_score const> score_tables::best_missions() const noexcept {
  /// Expose ranked mission records without allowing the interface to reorder saved entries
  return missions;
}
std::span<career_score const> score_tables::best_careers() const noexcept {
  /// Expose cumulative points alongside their original career statistics
  return careers;
}
std::span<career_score const> score_tables::fastest_careers() const noexcept {
  /// The table stores full points/counters; display can report the integer rate without losing the original totals
  return fastest;
}

} // namespace corncob::game

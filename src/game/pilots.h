#pragma once

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "game/statistics.h"

namespace corncob::game {

enum class pilot_rank : uint8_t {
  second_lieutenant,
  first_lieutenant,
  captain,
  major,
  lieutenant_colonel,
  colonel,
  general,
};
enum class medal : uint16_t {
  purple_heart = 0x257,
  bronze_star,
  silver_star,
  gold_star,
  distinguished_service_cross,
  service_award,
  stellar_cluster,
  combat_master,
  distinguished_flying_cross,
  theatre_combat,
};

class pilot_record {
private:
  std::array<uint8_t, 623> data{};
  uint16_t word(unsigned int offset) const;
  void word(unsigned int offset, uint16_t value);
  int32_t dword(unsigned int offset) const;
  void dword(unsigned int offset, int32_t value);

public:
  explicit pilot_record(std::string_view name = "trainee");
  explicit pilot_record(std::span<uint8_t const> source);
  std::string name() const;
  void rename(std::string_view name);
  bool selected() const;
  void select(bool value);
  bool unavailable() const;
  void resurrect();
  pilot_rank rank() const;
  uint16_t sorties() const;
  uint16_t resurrections() const;
  uint8_t awards(medal kind) const;
  int32_t career_score() const;
  sortie_statistics last_sortie() const;
  sortie_statistics career() const;
  std::span<uint8_t const> native_data() const noexcept;
  void finish_sortie(sortie_statistics const &result);
  void award(medal kind);
};

class pilot_roster {
private:
  std::vector<uint8_t> header = std::vector<uint8_t>(24);
  uint8_t retained_byte{0};
  uint16_t seed{1};

public:
  std::vector<pilot_record> pilots;
  static pilot_roster decode(std::span<uint8_t const> source);
  std::vector<uint8_t> encode() const;
  static void mask(std::span<uint8_t> bytes, uint16_t seed) noexcept;
};

std::string_view rank_name(pilot_rank rank) noexcept;

} // namespace corncob::game

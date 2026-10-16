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

struct medal_definition {
  medal kind;
  std::string_view name;
  std::array<char, 2> glyphs;
  uint8_t colour;
};

// original MOAG 3.42 award table at file offset 0x1fa82; artwork comes from MEDAL.FNC
inline std::array<medal_definition, 10> constexpr medal_definitions{
  medal_definition{
    .kind{medal::purple_heart},
    .name{"Purple Heart"},
    .glyphs{static_cast<char>(0xd0), static_cast<char>(0xe0)},
    .colour{13},
  },
  medal_definition{
    .kind{medal::bronze_star},
    .name{"Bronze Star"},
    .glyphs{static_cast<char>(0xd1), static_cast<char>(0xe1)},
    .colour{6},
  },
  medal_definition{
    .kind{medal::silver_star},
    .name{"Silver Star"},
    .glyphs{static_cast<char>(0xd1), static_cast<char>(0xe1)},
    .colour{7},
  },
  medal_definition{
    .kind{medal::gold_star},
    .name{"Gold Star"},
    .glyphs{static_cast<char>(0xd1), static_cast<char>(0xe1)},
    .colour{14},
  },
  medal_definition{
    .kind{medal::distinguished_service_cross},
    .name{"Distinguished Service Cross"},
    .glyphs{static_cast<char>(0xd2), static_cast<char>(0xe2)},
    .colour{6},
  },
  medal_definition{
    .kind{medal::service_award},
    .name{"Service Award"},
    .glyphs{static_cast<char>(0xd3), static_cast<char>(0xe3)},
    .colour{14},
  },
  medal_definition{
    .kind{medal::stellar_cluster},
    .name{"Stellar Cluster"},
    .glyphs{static_cast<char>(0xd4), static_cast<char>(0xe4)},
    .colour{15},
  },
  medal_definition{
    .kind{medal::combat_master},
    .name{"Combat Master"},
    .glyphs{static_cast<char>(0xd5), static_cast<char>(0xe5)},
    .colour{15},
  },
  medal_definition{
    .kind{medal::distinguished_flying_cross},
    .name{"Distinguished Flying Cross"},
    .glyphs{static_cast<char>(0xd6), static_cast<char>(0xe6)},
    .colour{14},
  },
  medal_definition{
    .kind{medal::theatre_combat},
    .name{"Theater Combat Medal"},
    .glyphs{static_cast<char>(0xd7), static_cast<char>(0xe7)},
    .colour{12},
  },
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
  std::span<uint8_t const> theatres() const;
  void add_theatre(uint8_t number);
  bool beginner_flight() const;
  void beginner_flight(bool value);
  void resurrect();
  pilot_rank rank() const;
  uint16_t sorties() const;
  uint16_t resurrections() const;
  uint8_t awards(medal kind) const;
  int32_t career_score() const;
  int32_t last_sortie_score() const;
  int32_t best_sortie_score() const;
  sortie_statistics last_sortie() const;
  sortie_statistics career() const;
  sortie_statistics best_sortie() const;
  sortie_statistics individual_records() const;
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
  bool keyboard_control() const;
  void keyboard_control(bool value);
  static pilot_roster decode(std::span<uint8_t const> source);
  std::vector<uint8_t> encode() const;
  static void mask(std::span<uint8_t> bytes, uint16_t seed) noexcept;
};

std::string_view rank_name(pilot_rank rank) noexcept;

} // namespace corncob::game

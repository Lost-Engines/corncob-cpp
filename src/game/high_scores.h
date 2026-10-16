#pragma once

#include "game/pilots.h"

namespace corncob::game {

using score_name = std::array<uint8_t, 22>;
struct mission_score {
  score_name name{};
  score_name theatre{};
  sortie_statistics statistics{};
  int32_t points{0};
  uint16_t retired{0};
};
struct career_score {
  score_name name{};
  uint16_t sorties{0};
  uint16_t resurrections{0};
  sortie_statistics statistics{};
  int32_t points{0};
  uint16_t retired{0};
};

class score_tables {
private:
  std::array<mission_score, 10> missions{};
  std::array<career_score, 10> careers{};
  std::array<career_score, 10> fastest{};

public:
  static unsigned int constexpr native_bytes{5040};
  static score_tables decode(std::span<uint8_t const> source);
  std::array<uint8_t, native_bytes> encode() const;
  void record(std::string_view theatre, sortie_statistics const &result, pilot_record const *pilot);
  std::span<mission_score const> best_missions() const noexcept;
  std::span<career_score const> best_careers() const noexcept;
  std::span<career_score const> fastest_careers() const noexcept;
};

std::string score_text(score_name const &value);

} // namespace corncob::game

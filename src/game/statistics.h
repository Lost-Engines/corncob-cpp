#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace corncob::game {

enum class sortie_status : uint16_t {
  killed = 0x0002,
  landed = 0x0004,
  captured = 0x0008,
  home = 0x0010,
  ejected = 0x0020,
  crashed_away = 0x0040,
  building_impact = 0x0080,
  rescued = 0x0100,
  crashed_home = 0x0200,
  earth_saved = 0x0400,
  invincible = 0x2000,
  aborted = 0x4000,
};
using quality_counts = std::array<uint16_t, 8>;

struct sortie_statistics {
  static unsigned int constexpr native_bytes{130};
  uint16_t status{0};
  uint16_t contacting_type{0};
  uint16_t aircraft_lost{0};
  uint16_t fuel_tet_kills{0};
  quality_counts kla_kills{};
  quality_counts aaa_kills{};
  quality_counts vehicle_kills{};
  quality_counts site_kills{};
  uint16_t objectives_completed{0};
  uint16_t friendly_destroyed{0};
  uint16_t maximum_damage{0};
  quality_counts peak_aaa{};
  quality_counts peak_kla{};
  std::array<uint16_t, 2> retained_words{};
  quality_counts orb_kills{};

  bool has(sortie_status flag) const noexcept;
  void mark(sortie_status flag) noexcept;
  std::array<uint8_t, native_bytes> encode() const;
  static sortie_statistics decode(std::span<uint8_t const> source);
};

int32_t calculate_score(sortie_statistics const &statistics);

} // namespace corncob::game

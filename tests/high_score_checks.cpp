#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "game/high_scores.h"
#include "maths/random.h"

namespace {

void require(bool const condition, char const *const message) {
  /// Check ranking and native file integrity independently of debug assertions
  if(!condition) throw std::runtime_error{message};
}

} // anonymous namespace

int main() {
  /// Preserve all 5040 original-format bytes, update repeated careers and enforce fastest-career eligibility
  namespace game = corncob::game;
  std::array<uint8_t, game::score_tables::native_bytes> original;
  corncob::maths::random_source random;
  for(auto &byte : original) byte = static_cast<uint8_t>(random.next());
  require(game::score_tables::decode(original).encode() == original, "high-score codec discarded record or padding bytes");
  game::score_tables tables;
  game::pilot_record alba{"Alba"}, birch{"Birch"};
  game::sortie_statistics result{
    .objectives_completed{1},
  };
  result.mark(game::sortie_status::home);
  result.mark(game::sortie_status::landed);
  for(unsigned int flight{0}; flight != 2; ++flight) {
    alba.finish_sortie(result);
    tables.record("morning", result, &alba);
  }
  require(tables.fastest_careers().front().points == 0, "fastest-career list accepted fewer than three missions");
  alba.finish_sortie(result);
  tables.record("dusk", result, &alba);
  require(game::score_text(tables.best_careers().front().name) == "Alba" && tables.best_careers().front().points == 5625, "career totals or rank changed");
  require(tables.fastest_careers().front().sorties == 3 && tables.fastest_careers().front().points == 5625, "third sortie did not qualify for fastest-career ranking");
  require(std::ranges::count_if(tables.best_careers(), [](auto const &entry){return game::score_text(entry.name) == "Alba";}) == 1,
    "repeated sorties duplicated the same career");
  result.objectives_completed = 2;
  birch.finish_sortie(result);
  tables.record("wasteland", result, &birch);
  require(game::score_text(tables.best_missions().front().name) == "Birch" && tables.best_missions().front().points == 3750, "best-mission ranking lost a larger score");
  require(game::score_text(tables.best_careers().front().name) == "Alba", "mission and career ranking were conflated");
  require(game::score_tables::decode(tables.encode()).encode() == tables.encode(), "ranked score tables did not survive saving");
  std::cout << "Native high-score record preservation, career replacement and three-sortie eligibility pass" << std::endl;
  return EXIT_SUCCESS;
}

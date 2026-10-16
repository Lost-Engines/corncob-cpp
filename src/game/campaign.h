#pragma once

#include <filesystem>
#include <optional>
#include "game/pilots.h"
#include "game/high_scores.h"
#include "world/world.h"

namespace corncob::game {

class campaign_store {
private:
  std::filesystem::path directory;
  pilot_roster roster;
  score_tables scores;
  std::optional<unsigned int> selected;

  std::filesystem::path theatre_path(std::string_view theatre) const;
  void save_roster() const;
  void save_scores(std::string_view theatre, sortie_statistics const &result);

public:
  explicit campaign_store(std::filesystem::path directory);
  std::span<pilot_record const> pilots() const noexcept;
  pilot_record const *active_pilot() const noexcept;
  score_tables const &high_scores() const noexcept;
  unsigned int create_pilot(std::string_view name);
  void activate(unsigned int index);
  void deactivate();
  void rename_pilot(unsigned int index, std::string_view name);
  void resurrect(unsigned int index);
  std::optional<world::theatre> load_world(std::string_view theatre) const;
  void finish_sortie(std::string_view theatre, world::theatre const &world, sortie_statistics const &result);
  std::filesystem::path const &save_directory() const noexcept;
};

} // namespace corncob::game

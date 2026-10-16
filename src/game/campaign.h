#pragma once

#include <filesystem>
#include <optional>
#include "game/pilots.h"
#include "game/high_scores.h"
#include "world/world.h"

namespace corncob::game {

struct saved_theatre {
  uint8_t number;
  std::string definition;
  std::string title;
  int16_t objectives_remaining;
  pilot_record pilot;
  unsigned int stored_aircraft;
};

class campaign_store {
private:
  std::filesystem::path directory;
  pilot_roster roster;
  score_tables scores;
  std::optional<unsigned int> selected;
  struct trainee_theatre {
    std::string definition;
    world::theatre state;
    unsigned int reserve_aircraft;
  };
  std::vector<trainee_theatre> trainee_theatres;
  std::optional<unsigned int> trainee_active_theatre;

  std::filesystem::path theatre_path(std::string_view theatre) const;
  void save_roster() const;
  void save_scores(std::string_view theatre, sortie_statistics const &result);

public:
  explicit campaign_store(std::filesystem::path directory);
  std::span<pilot_record const> pilots() const noexcept;
  pilot_record const *active_pilot() const noexcept;
  bool can_fly() const;
  score_tables const &high_scores() const noexcept;
  unsigned int create_pilot(std::string_view name, bool activate = true);
  void activate(unsigned int index);
  void deactivate();
  void rename_pilot(unsigned int index, std::string_view name);
  void resurrect(unsigned int index);
  void delete_pilot(unsigned int index);
  bool music_enabled() const noexcept;
  void music_enabled(bool value);
  bool keyboard_control() const noexcept;
  void keyboard_control(bool value);
  void beginner_flight(bool value);
  world::training_setup training_settings() const;
  void training_settings(world::training_setup const &settings);
  std::vector<saved_theatre> theatres() const;
  std::optional<unsigned int> active_theatre() const;
  void open_theatre(std::string_view theatre, world::theatre const &world);
  void activate_theatre(unsigned int index);
  void delete_theatre(unsigned int index);
  bool combat_invincible() const;
  void combat_invincible(bool value);
  bool inflight_comments() const;
  void inflight_comments(bool value);
  void clear_scores(unsigned int list);
  std::optional<world::theatre> load_world(std::string_view theatre) const;
  void finish_sortie(std::string_view theatre, world::theatre const &world, sortie_statistics const &result, bool reserve_aircraft = false);
  std::filesystem::path const &save_directory() const noexcept;
};

} // namespace corncob::game

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include "game/campaign.h"
#include "resources/assets.h"
#include "game/session.h"

namespace {

void require(bool const condition, char const *const message) {
  /// Keep persistence checks active in optimised builds
  if(!condition) throw std::runtime_error{message};
}

} // anonymous namespace

int main(int const argc, char const *const argv[]) {
  /// Persist a named career and changed original world, reopen it, and keep different pilots' worlds separate
  if(argc != 2) throw std::invalid_argument{"campaign_checks requires its dedicated scratch directory"};
  std::filesystem::path const directory{argv[1]};
  if(directory.filename() != "campaign-checks") throw std::invalid_argument{"campaign check directory must be named campaign-checks"};
  std::filesystem::remove_all(directory);
  namespace game = corncob::game;
  namespace world = corncob::world;
  auto theatre{world::decode_theatre(corncob::resources::morning_asset())};
  auto orb{std::ranges::find_if(theatre.objects, [](auto const &object){return object.type == world::object_type::generator_orb;})};
  require(orb != theatre.objects.end(), "reference theatre lacks a generator orb");
  orb->type = world::object_type::destroyed_generator_orb;
  orb->flags = 0x211;
  {
    game::campaign_store campaign{directory};
    require(campaign.active_pilot() == nullptr, "empty roster did not begin with disposable trainee");
    campaign.create_pilot("Native pilot");
    require(campaign.active_pilot()->name() == "Native pilot", "new pilot was not active");
    game::sortie_statistics result{
      .objectives_completed{1},
    };
    result.mark(game::sortie_status::home);
    result.mark(game::sortie_status::landed);
    campaign.finish_sortie("morning", theatre, result);
  }
  {
    game::campaign_store campaign{directory};
    require(campaign.active_pilot() != nullptr && campaign.active_pilot()->rank() == game::pilot_rank::first_lieutenant, "saved pilot progression was lost on restart");
    require(campaign.active_pilot()->career_score() == 1875, "saved career score changed");
    auto const loaded{campaign.load_world("morning")};
    require(loaded.has_value(), "changed theatre was not saved");
    require(std::ranges::any_of(loaded->objects, [](auto const &object){return object.type == world::object_type::destroyed_generator_orb && (object.flags & 0x200u) != 0;}), "destroyed objective reverted after loading");
    campaign.rename_pilot(0, "Renamed pilot");
    require(campaign.load_world("morning").has_value(), "renaming orphaned the pilot's theatre");
    campaign.create_pilot("Other pilot");
    require(!campaign.load_world("morning"), "new pilot inherited another career's world");
    campaign.activate(0);
    require(campaign.load_world("morning").has_value(), "switching pilots lost the original saved world");
    campaign.deactivate();
    require(campaign.active_pilot() == nullptr && !campaign.load_world("morning"), "trainee retained named pilot state");
  }
  {
    game::campaign_store campaign{directory};
    campaign.activate(0);
    auto finish_world{[&](std::string_view const name, std::span<uint8_t const> const asset){
      auto world{world::decode_theatre(asset)};
      for(auto &object : world.objects) object.flags |= std::to_underlying(world::object_flag::objective_complete);
      game::sortie_statistics result{
        .objectives_completed{4},
      };
      result.mark(game::sortie_status::home);
      result.mark(game::sortie_status::landed);
      campaign.finish_sortie(name, world, result);
    }};
    finish_world("morning", corncob::resources::morning_asset());
    finish_world("dusk", corncob::resources::dusk_asset());
    require(campaign.active_pilot()->awards(game::medal::combat_master) == 0, "clearance awarded before the third theatre");
    finish_world("wasteland", corncob::resources::wasteland_asset());
    require(campaign.active_pilot()->awards(game::medal::combat_master) == 1 && campaign.active_pilot()->rank() == game::pilot_rank::captain,
      "clearing the required theatres did not award Captain/Combat Master clearance");
    auto const awards{campaign.active_pilot()->awards(game::medal::theatre_combat)};
    finish_world("wasteland", corncob::resources::wasteland_asset());
    require(campaign.active_pilot()->awards(game::medal::theatre_combat) == awards, "a completed theatre repeatedly awarded medals");
    auto sands{world::decode_theatre(corncob::resources::white_sands_asset())};
    auto const plane{std::ranges::find_if(sands.objects, [](auto const &object){return object.type == world::object_type::aircraft && (object.flags & 0x2000u) != 0;})};
    sands.start = corncob::maths::world_position{plane->position};
    sands.start.y += 3000;
    corncob::resources::geometry_bank geometry{corncob::resources::engine_data_asset()};
    game::session ending{geometry, std::move(sands)};
    ending.set_experimental_clearance(campaign.active_pilot()->awards(game::medal::combat_master) != 0);
    ending.advance({});
    require(ending.player() == game::player_mode::in_aircraft, "ending pilot could not enter the experimental aircraft");
    ending.relocate({
      .title{"High altitude test"},
      .position{0, 0, 1'000'000},
      .heading{0},
    });
    ending.advance({});
    require(ending.player() == game::player_mode::earth_saved && ending.finished(), "cleared experimental high-altitude flight did not reach the ending");
    campaign.finish_sortie("white_sands", ending.snapshot(), ending.results());
    require(campaign.active_pilot()->rank() == game::pilot_rank::major, "saving Earth did not award the normal game's final rank");
  }
  std::filesystem::remove_all(directory);
  std::cout << "Native masked roster, career progression and independent live theatre persistence survive restart" << std::endl;
  return EXIT_SUCCESS;
}

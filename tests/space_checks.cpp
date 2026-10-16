#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include "game/front_end.h"
#include "graphics/space_sequence.h"
#include "resources/assets.h"
#include "resources/binary_reader.h"

int main(int const argc, char const *const argv[]) {
  /// Compare unchanged SRATS initialization and every indexed star pixel, then exercise its retained-flight ending flow
  if(argc != 2) throw std::invalid_argument{"space reference path required"};
  std::ifstream input{argv[1], std::ios::binary};
  std::vector<uint8_t> bytes{std::istreambuf_iterator<char>{input}, {}};
  corncob::resources::binary_reader const data{bytes};
  if(bytes.size() != 5136 || std::string{bytes.begin(), bytes.begin() + 8} != "CORNSTA1") throw std::runtime_error{"invalid space fixture"};
  corncob::graphics::star_field stars;
  unsigned int offset{16};
  for(auto const &star : stars.positions()) {
    if(star.x != data.signed_word(offset) || star.y != data.signed_word(offset + 2) || star.z != data.word(offset + 4)) throw std::runtime_error{"native star initialization mismatch"};
    offset += 6;
  }
  for(unsigned int frame{0}; frame != 64; ++frame) {
    corncob::graphics::indexed_framebuffer target{};
    stars.advance(target);
    uint64_t hash{14'695'981'039'346'656'037ull};
    for(auto const pixel : target.pixels) hash = (hash ^ pixel) * 1'099'511'628'211ull;
    uint64_t const expected{data.dword(offset) | (static_cast<uint64_t>(data.dword(offset + 4)) << 32)};
    if(hash != expected) throw std::runtime_error{"native star projection mismatch at frame " + std::to_string(frame)};
    offset += 8;
  }
  corncob::graphics::indexed_framebuffer backdrop{};
  backdrop.pixels.fill(5);
  std::array<corncob::render::rgba_pixel, 16> palette{};
  for(unsigned int i{0}; i != 16; ++i) palette[i] = {static_cast<uint8_t>(i * 17), 0, 0};
  corncob::graphics::space_sequence sequence{backdrop, palette};
  corncob::render::framebuffer target;
  sequence.advance(10.0);
  sequence.render(target);
  if(target.width != 640 || target.height != 350 || target.pixels[220 * 640 + 10].red != 85 || target.pixels[100 * 640 + 10].red != 0) throw std::runtime_error{"ending lost its cockpit or sky fade"};
  if(sequence.finished()) throw std::runtime_error{"idle starfield must remain interactive"};
  sequence.toggle_warp();
  sequence.advance(24.0);
  if(!sequence.finished()) throw std::runtime_error{"native warp flash/fade did not complete"};
  namespace game = corncob::game;
  auto const scratch{std::filesystem::temp_directory_path() / "corncob-ending-checks"};
  std::filesystem::remove_all(scratch);
  game::campaign_store career{scratch};
  corncob::resources::collection assets{corncob::resources::front_end_asset()};
  game::front_end front{assets, career, true};
  game::sortie_statistics result;
  result.status = std::to_underlying(game::sortie_status::earth_saved);
  front.finish_sortie(result);
  front.begin_ending(backdrop, palette);
  front.advance(10.0);
  front.command(game::interface_command::accept);
  front.render(target);
  if(front.current_page() != game::interface_page::ending || front.in_flight()) throw std::runtime_error{"ending resumed its already finished sortie"};
  front.command(game::interface_command::cancel);
  if(front.current_page() != game::interface_page::outcome) throw std::runtime_error{"ending did not reach Earth-saved outcome"};
  front.command(game::interface_command::accept);
  if(front.current_page() != game::interface_page::sortie_summary) throw std::runtime_error{"ending lost the recorded sortie report"};
  std::filesystem::remove_all(scratch);
  std::cout << "768 original stars, 64 native pixel frames and unified ending exercised" << std::endl;
  return EXIT_SUCCESS;
}

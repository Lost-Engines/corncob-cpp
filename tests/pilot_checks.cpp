#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "game/pilots.h"
#include "tests/reference_reader.h"

int main(int const argc, char const *const argv[]) {
  /// Check original masking, opaque-byte preservation and both retained native roster layouts
  if(argc != 2) throw std::invalid_argument{"pilot_checks requires a fixture path"};
  corncob::tests::reference_reader input{argv[1], "CORNPLT1"};
  auto const cases{input.dword()};
  for(unsigned int i{0}; i != cases; ++i) {
    auto const seed{input.word()}, bytes{input.word()};
    std::vector<uint8_t> actual(bytes);
    corncob::game::pilot_roster::mask(actual, seed);
    for(auto const byte : actual) if(byte != input.byte()) throw std::runtime_error{"native pilot masking stream differs"};
  }
  for(unsigned int const header : {23u, 24u}) {
    for(unsigned int count{0}; count != 4; ++count) {
      std::vector<uint8_t> plain(header, 0x71);
      plain[header - 2] = static_cast<uint8_t>(count);
      plain[header - 1] = 0;
      for(unsigned int i{0}; i != count; ++i) {
        std::array<uint8_t, 623> record;
        for(unsigned int n{0}; n != record.size(); ++n) record[n] = static_cast<uint8_t>(n * 73 + i);
        std::fill_n(record.begin(), 22, 0);
        std::string const name{"Native test " + std::to_string(i)};
        std::ranges::copy(name, record.begin());
        plain.insert(plain.end(), record.begin(), record.end());
        if(i == 0) plain.push_back(0x93);
      }
      if(count == 0) plain.push_back(0x93);
      corncob::game::pilot_roster::mask(plain, 73);
      std::vector<uint8_t> encoded{24, 0, 73, 0};
      encoded.insert(encoded.end(), plain.begin(), plain.end());
      auto const roster{corncob::game::pilot_roster::decode(encoded)};
      if(roster.pilots.size() != count || roster.encode() != encoded) throw std::runtime_error{"native pilot roster did not round trip"};
    }
  }
  corncob::game::pilot_record pilot{"Native test"};
  corncob::game::sortie_statistics result{
    .objectives_completed{7},
  };
  pilot.finish_sortie(result);
  if(pilot.rank() != corncob::game::pilot_rank::captain || pilot.career().objectives_completed != 7)
    throw std::runtime_error{"normal captain promotion or cumulative objectives failed"};
  if(pilot.awards(corncob::game::medal::distinguished_service_cross) != 1) throw std::runtime_error{"five-objective medal failed"};
  std::cout << "16384 native pilot-mask bytes, both roster layouts and basic career progression exercised" << std::endl;
  return EXIT_SUCCESS;
}

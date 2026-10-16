#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "game/statistics.h"
#include "tests/reference_reader.h"

int main(int const argc, char const *const argv[]) {
  /// Compare unchanged native score arithmetic and verify lossless result-block serialization
  if(argc != 2) throw std::invalid_argument{"score_checks requires a fixture path"};
  corncob::tests::reference_reader input{argv[1], "CORNSCR1"};
  auto const cases{input.dword()};
  for(unsigned int i{0}; i != cases; ++i) {
    std::array<uint8_t, corncob::game::sortie_statistics::native_bytes> data;
    for(auto &byte : data) byte = input.byte();
    auto const statistics{corncob::game::sortie_statistics::decode(data)};
    if(statistics.encode() != data) throw std::runtime_error{"native sortie record did not round trip"};
    auto const expected{input.signed_dword()};
    auto const actual{corncob::game::calculate_score(statistics)};
    if(actual != expected) throw std::runtime_error{"native score case " + std::to_string(i) + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual)};
  }
  std::cout << cases << " native sortie scores and result-record round trips match" << std::endl;
  return EXIT_SUCCESS;
}

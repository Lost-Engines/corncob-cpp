#pragma once

#include <array>
#include <cstdint>

namespace corncob::resources {

enum class colour_rule_kind { random, alternate, radar, flag_blink, quality, quality_random };

struct colour_rule {
  unsigned int address;
  colour_rule_kind kind;
  std::array<uint16_t, 8> values;
  uint64_t types;
  uint64_t random_types;
  uint16_t flag_mask;
  uint16_t timer_mask;
};

// generated from the native-verified colour profile; addresses refer only to embedded data
inline std::array<colour_rule, 21> constexpr colour_rules{
  colour_rule{
    .address{0x31b6},
    .kind{colour_rule_kind::random},
    .values{0, 0, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x2dc2},
    .kind{colour_rule_kind::radar},
    .values{0, 15, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0400},
  },
  colour_rule{
    .address{0x2dc3},
    .kind{colour_rule_kind::alternate},
    .values{14, 12, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x2dc4},
    .kind{colour_rule_kind::alternate},
    .values{12, 14, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x2dc5},
    .kind{colour_rule_kind::alternate},
    .values{14, 1, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x2dc6},
    .kind{colour_rule_kind::alternate},
    .values{1, 14, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x2dc7},
    .kind{colour_rule_kind::alternate},
    .values{4, 0, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x2dc8},
    .kind{colour_rule_kind::alternate},
    .values{0, 4, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x31b7},
    .kind{colour_rule_kind::quality},
    .values{0, 6, 7, 3, 9, 13, 4, 14},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x31b8},
    .kind{colour_rule_kind::flag_blink},
    .values{0, 15, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0800},
    .timer_mask{0x0200},
  },
  colour_rule{
    .address{0x6200},
    .kind{colour_rule_kind::quality},
    .values{22, 3, 6, 7, 4, 13, 14, 15},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x6386},
    .kind{colour_rule_kind::quality_random},
    .values{22, 3, 6, 7, 4, 13, 14, 15},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x7652},
    .kind{colour_rule_kind::quality},
    .values{0, 6, 7, 3, 9, 13, 11, 2},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x601a},
    .kind{colour_rule_kind::quality},
    .values{7, 7, 7, 7, 3, 3, 3, 15},
    .types{0x1000ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x601c},
    .kind{colour_rule_kind::quality},
    .values{6, 6, 1, 0, 6, 1, 0, 0},
    .types{0x1000ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x601a},
    .kind{colour_rule_kind::quality},
    .values{7, 7, 7, 7, 3, 3, 3, 15},
    .types{0x2000ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x601e},
    .kind{colour_rule_kind::flag_blink},
    .values{0, 15, 0, 0, 0, 0, 0, 0},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0800},
    .timer_mask{0x0200},
  },
  colour_rule{
    .address{0x68e0},
    .kind{colour_rule_kind::quality},
    .values{0, 6, 6, 7, 7, 7, 1, 1},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x68e2},
    .kind{colour_rule_kind::quality},
    .values{0, 6, 6, 7, 7, 7, 1, 1},
    .types{0x0ull},
    .random_types{0x20000000000000ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x68e1},
    .kind{colour_rule_kind::quality},
    .values{15, 14, 15, 9, 14, 15, 14, 15},
    .types{0x0ull},
    .random_types{0x0ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
  colour_rule{
    .address{0x68e3},
    .kind{colour_rule_kind::quality},
    .values{15, 14, 15, 9, 14, 15, 14, 15},
    .types{0x0ull},
    .random_types{0x20000000000000ull},
    .flag_mask{0x0000},
    .timer_mask{0x0000},
  },
};

} // namespace corncob::resources

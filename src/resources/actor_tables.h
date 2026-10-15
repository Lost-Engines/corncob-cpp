#pragma once

#include <array>
#include <cstdint>

namespace corncob::resources {

// generated from the original 3.42 quality tables; qualities are stored as indices 0..7

struct kla_parameters {
  uint16_t colour;
  uint16_t lock_colour;
  uint16_t heading_turn_limit;
  uint16_t pitch_turn_limit;
  uint16_t speed_relaxation_divisor;
  uint16_t locked_target_speed;
  uint16_t normal_target_speed;
  uint16_t off_axis_target_speed;
  uint16_t speed_noise_span;
  uint16_t lock_angle_limit;
  uint16_t off_axis_angle_limit;
  uint16_t roll_rate;
};

inline std::array<kla_parameters, 8> constexpr kla_qualities{
  kla_parameters{
    .colour{6},
    .lock_colour{9},
    .heading_turn_limit{50},
    .pitch_turn_limit{50},
    .speed_relaxation_divisor{60},
    .locked_target_speed{800},
    .normal_target_speed{550},
    .off_axis_target_speed{550},
    .speed_noise_span{31},
    .lock_angle_limit{1500},
    .off_axis_angle_limit{16384},
    .roll_rate{800},
  },
  kla_parameters{
    .colour{6},
    .lock_colour{14},
    .heading_turn_limit{50},
    .pitch_turn_limit{50},
    .speed_relaxation_divisor{120},
    .locked_target_speed{1100},
    .normal_target_speed{800},
    .off_axis_target_speed{550},
    .speed_noise_span{31},
    .lock_angle_limit{1000},
    .off_axis_angle_limit{16384},
    .roll_rate{900},
  },
  kla_parameters{
    .colour{6},
    .lock_colour{15},
    .heading_turn_limit{70},
    .pitch_turn_limit{70},
    .speed_relaxation_divisor{120},
    .locked_target_speed{800},
    .normal_target_speed{650},
    .off_axis_target_speed{250},
    .speed_noise_span{31},
    .lock_angle_limit{1000},
    .off_axis_angle_limit{16384},
    .roll_rate{1000},
  },
  kla_parameters{
    .colour{7},
    .lock_colour{9},
    .heading_turn_limit{90},
    .pitch_turn_limit{90},
    .speed_relaxation_divisor{120},
    .locked_target_speed{800},
    .normal_target_speed{650},
    .off_axis_target_speed{50},
    .speed_noise_span{81},
    .lock_angle_limit{500},
    .off_axis_angle_limit{16384},
    .roll_rate{1200},
  },
  kla_parameters{
    .colour{7},
    .lock_colour{14},
    .heading_turn_limit{90},
    .pitch_turn_limit{90},
    .speed_relaxation_divisor{120},
    .locked_target_speed{900},
    .normal_target_speed{750},
    .off_axis_target_speed{250},
    .speed_noise_span{11},
    .lock_angle_limit{500},
    .off_axis_angle_limit{16384},
    .roll_rate{1300},
  },
  kla_parameters{
    .colour{7},
    .lock_colour{15},
    .heading_turn_limit{100},
    .pitch_turn_limit{100},
    .speed_relaxation_divisor{180},
    .locked_target_speed{1200},
    .normal_target_speed{800},
    .off_axis_target_speed{550},
    .speed_noise_span{11},
    .lock_angle_limit{500},
    .off_axis_angle_limit{16384},
    .roll_rate{1400},
  },
  kla_parameters{
    .colour{1},
    .lock_colour{14},
    .heading_turn_limit{120},
    .pitch_turn_limit{120},
    .speed_relaxation_divisor{120},
    .locked_target_speed{1000},
    .normal_target_speed{750},
    .off_axis_target_speed{250},
    .speed_noise_span{21},
    .lock_angle_limit{200},
    .off_axis_angle_limit{16384},
    .roll_rate{1500},
  },
  kla_parameters{
    .colour{1},
    .lock_colour{15},
    .heading_turn_limit{150},
    .pitch_turn_limit{150},
    .speed_relaxation_divisor{60},
    .locked_target_speed{1200},
    .normal_target_speed{825},
    .off_axis_target_speed{250},
    .speed_noise_span{11},
    .lock_angle_limit{500},
    .off_axis_angle_limit{8192},
    .roll_rate{1700},
  },
};

struct kla_site_parameters {
  uint16_t colour;
  uint16_t colour_2;
  uint16_t launch_reset;
};

inline std::array<kla_site_parameters, 8> constexpr kla_site_qualities{
  kla_site_parameters{
    .colour{6},
    .colour_2{9},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{6},
    .colour_2{14},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{6},
    .colour_2{15},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{7},
    .colour_2{9},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{7},
    .colour_2{14},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{7},
    .colour_2{15},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{1},
    .colour_2{14},
    .launch_reset{400},
  },
  kla_site_parameters{
    .colour{1},
    .colour_2{15},
    .launch_reset{400},
  },
};

struct aaa_parameters {
  uint16_t colour;
  uint16_t fire_threshold;
  uint16_t shell_jitter;
  uint16_t heading_turn_limit;
  uint16_t pitch_turn_limit;
  int16_t lead_target;
};

inline std::array<aaa_parameters, 8> constexpr aaa_qualities{
  aaa_parameters{
    .colour{6},
    .fire_threshold{500},
    .shell_jitter{100},
    .heading_turn_limit{30},
    .pitch_turn_limit{30},
    .lead_target{0},
  },
  aaa_parameters{
    .colour{5},
    .fire_threshold{1000},
    .shell_jitter{50},
    .heading_turn_limit{50},
    .pitch_turn_limit{50},
    .lead_target{0},
  },
  aaa_parameters{
    .colour{7},
    .fire_threshold{1000},
    .shell_jitter{50},
    .heading_turn_limit{50},
    .pitch_turn_limit{50},
    .lead_target{0},
  },
  aaa_parameters{
    .colour{7},
    .fire_threshold{1500},
    .shell_jitter{30},
    .heading_turn_limit{50},
    .pitch_turn_limit{50},
    .lead_target{-1},
  },
  aaa_parameters{
    .colour{3},
    .fire_threshold{1500},
    .shell_jitter{30},
    .heading_turn_limit{50},
    .pitch_turn_limit{50},
    .lead_target{-1},
  },
  aaa_parameters{
    .colour{3},
    .fire_threshold{2000},
    .shell_jitter{30},
    .heading_turn_limit{70},
    .pitch_turn_limit{70},
    .lead_target{-1},
  },
  aaa_parameters{
    .colour{15},
    .fire_threshold{3000},
    .shell_jitter{10},
    .heading_turn_limit{120},
    .pitch_turn_limit{120},
    .lead_target{-1},
  },
  aaa_parameters{
    .colour{15},
    .fire_threshold{4000},
    .shell_jitter{10},
    .heading_turn_limit{500},
    .pitch_turn_limit{500},
    .lead_target{-1},
  },
};

struct alien_parameters {
  uint16_t colour;
  uint16_t gravity;
  uint16_t jump_velocity;
  uint16_t horizontal_jitter;
};

inline std::array<alien_parameters, 8> constexpr alien_qualities{
  alien_parameters{
    .colour{5},
    .gravity{3},
    .jump_velocity{50},
    .horizontal_jitter{50},
  },
  alien_parameters{
    .colour{3},
    .gravity{3},
    .jump_velocity{50},
    .horizontal_jitter{100},
  },
  alien_parameters{
    .colour{6},
    .gravity{2},
    .jump_velocity{30},
    .horizontal_jitter{75},
  },
  alien_parameters{
    .colour{7},
    .gravity{2},
    .jump_velocity{50},
    .horizontal_jitter{75},
  },
  alien_parameters{
    .colour{4},
    .gravity{1},
    .jump_velocity{50},
    .horizontal_jitter{100},
  },
  alien_parameters{
    .colour{13},
    .gravity{3},
    .jump_velocity{150},
    .horizontal_jitter{150},
  },
  alien_parameters{
    .colour{14},
    .gravity{2},
    .jump_velocity{100},
    .horizontal_jitter{200},
  },
  alien_parameters{
    .colour{15},
    .gravity{3},
    .jump_velocity{500},
    .horizontal_jitter{300},
  },
};

struct forcefield_parameters {
  uint16_t colour;
};

inline std::array<forcefield_parameters, 8> constexpr forcefield_qualities{
  forcefield_parameters{
    .colour{22},
  },
  forcefield_parameters{
    .colour{3},
  },
  forcefield_parameters{
    .colour{6},
  },
  forcefield_parameters{
    .colour{7},
  },
  forcefield_parameters{
    .colour{4},
  },
  forcefield_parameters{
    .colour{13},
  },
  forcefield_parameters{
    .colour{14},
  },
  forcefield_parameters{
    .colour{15},
  },
};

struct portal_parameters {
  uint16_t colour;
  uint16_t flash_colour;
  uint16_t action_bits;
  int16_t damage_or_repair;
};

inline std::array<portal_parameters, 8> constexpr portal_qualities{
  portal_parameters{
    .colour{11},
    .flash_colour{27},
    .action_bits{1},
    .damage_or_repair{0},
  },
  portal_parameters{
    .colour{3},
    .flash_colour{25},
    .action_bits{255},
    .damage_or_repair{0},
  },
  portal_parameters{
    .colour{12},
    .flash_colour{27},
    .action_bits{1},
    .damage_or_repair{1},
  },
  portal_parameters{
    .colour{0},
    .flash_colour{27},
    .action_bits{1},
    .damage_or_repair{-1},
  },
  portal_parameters{
    .colour{11},
    .flash_colour{27},
    .action_bits{257},
    .damage_or_repair{0},
  },
  portal_parameters{
    .colour{11},
    .flash_colour{11},
    .action_bits{1},
    .damage_or_repair{1},
  },
  portal_parameters{
    .colour{7},
    .flash_colour{27},
    .action_bits{0},
    .damage_or_repair{2},
  },
  portal_parameters{
    .colour{10},
    .flash_colour{3},
    .action_bits{256},
    .damage_or_repair{0},
  },
};

struct vehicle_site_parameters {
  uint16_t colour;
  uint16_t launch_reset;
};

inline std::array<vehicle_site_parameters, 8> constexpr vehicle_site_qualities{
  vehicle_site_parameters{
    .colour{7},
    .launch_reset{150},
  },
  vehicle_site_parameters{
    .colour{7},
    .launch_reset{200},
  },
  vehicle_site_parameters{
    .colour{7},
    .launch_reset{200},
  },
  vehicle_site_parameters{
    .colour{7},
    .launch_reset{200},
  },
  vehicle_site_parameters{
    .colour{3},
    .launch_reset{200},
  },
  vehicle_site_parameters{
    .colour{3},
    .launch_reset{300},
  },
  vehicle_site_parameters{
    .colour{3},
    .launch_reset{300},
  },
  vehicle_site_parameters{
    .colour{15},
    .launch_reset{300},
  },
};

struct vehicle_parameters {
  uint16_t body_colour;
  uint16_t panel_colour;
  uint16_t far_speed;
  uint16_t near_speed;
  uint16_t far_pursuit_threshold;
  uint16_t near_pursuit_threshold;
  uint16_t change_direction_threshold;
  uint16_t weave_frequency;
  uint16_t weave_divisor;
};

inline std::array<vehicle_parameters, 8> constexpr vehicle_qualities{
  vehicle_parameters{
    .body_colour{7},
    .panel_colour{6},
    .far_speed{250},
    .near_speed{100},
    .far_pursuit_threshold{6000},
    .near_pursuit_threshold{8000},
    .change_direction_threshold{500},
    .weave_frequency{21},
    .weave_divisor{15},
  },
  vehicle_parameters{
    .body_colour{7},
    .panel_colour{6},
    .far_speed{300},
    .near_speed{150},
    .far_pursuit_threshold{6000},
    .near_pursuit_threshold{8000},
    .change_direction_threshold{500},
    .weave_frequency{10},
    .weave_divisor{65},
  },
  vehicle_parameters{
    .body_colour{7},
    .panel_colour{1},
    .far_speed{300},
    .near_speed{200},
    .far_pursuit_threshold{6000},
    .near_pursuit_threshold{8000},
    .change_direction_threshold{500},
    .weave_frequency{7},
    .weave_divisor{120},
  },
  vehicle_parameters{
    .body_colour{7},
    .panel_colour{0},
    .far_speed{300},
    .near_speed{300},
    .far_pursuit_threshold{6000},
    .near_pursuit_threshold{8000},
    .change_direction_threshold{500},
    .weave_frequency{21},
    .weave_divisor{50},
  },
  vehicle_parameters{
    .body_colour{3},
    .panel_colour{6},
    .far_speed{400},
    .near_speed{400},
    .far_pursuit_threshold{5000},
    .near_pursuit_threshold{32000},
    .change_direction_threshold{300},
    .weave_frequency{10},
    .weave_divisor{70},
  },
  vehicle_parameters{
    .body_colour{3},
    .panel_colour{1},
    .far_speed{500},
    .near_speed{450},
    .far_pursuit_threshold{7000},
    .near_pursuit_threshold{32000},
    .change_direction_threshold{200},
    .weave_frequency{19},
    .weave_divisor{40},
  },
  vehicle_parameters{
    .body_colour{3},
    .panel_colour{0},
    .far_speed{700},
    .near_speed{600},
    .far_pursuit_threshold{1000},
    .near_pursuit_threshold{32000},
    .change_direction_threshold{2000},
    .weave_frequency{4},
    .weave_divisor{50},
  },
  vehicle_parameters{
    .body_colour{15},
    .panel_colour{0},
    .far_speed{800},
    .near_speed{600},
    .far_pursuit_threshold{2000},
    .near_pursuit_threshold{32000},
    .change_direction_threshold{3000},
    .weave_frequency{10},
    .weave_divisor{65},
  },
};

struct balloon_parameters {
  uint16_t colour;
  uint16_t vertical_launch_speed;
  uint16_t reversal_threshold;
  uint16_t word_3_untraced;
};

inline std::array<balloon_parameters, 8> constexpr balloon_qualities{
  balloon_parameters{
    .colour{0},
    .vertical_launch_speed{150},
    .reversal_threshold{10},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{6},
    .vertical_launch_speed{200},
    .reversal_threshold{20},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{7},
    .vertical_launch_speed{200},
    .reversal_threshold{40},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{3},
    .vertical_launch_speed{250},
    .reversal_threshold{80},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{9},
    .vertical_launch_speed{350},
    .reversal_threshold{160},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{13},
    .vertical_launch_speed{500},
    .reversal_threshold{320},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{11},
    .vertical_launch_speed{600},
    .reversal_threshold{60},
    .word_3_untraced{2000},
  },
  balloon_parameters{
    .colour{2},
    .vertical_launch_speed{800},
    .reversal_threshold{500},
    .word_3_untraced{1000},
  },
};

struct saucer_site_parameters {
  uint16_t colour;
  uint16_t colour_2;
  uint16_t launch_reset;
};

inline std::array<saucer_site_parameters, 8> constexpr saucer_site_qualities{
  saucer_site_parameters{
    .colour{0},
    .colour_2{15},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{6},
    .colour_2{14},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{6},
    .colour_2{15},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{7},
    .colour_2{9},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{7},
    .colour_2{14},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{7},
    .colour_2{15},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{1},
    .colour_2{14},
    .launch_reset{400},
  },
  saucer_site_parameters{
    .colour{1},
    .colour_2{15},
    .launch_reset{400},
  },
};

struct shell_contents_parameters {
  uint16_t spawn_type;
  uint16_t colour_1;
  uint16_t colour_2;
};

inline std::array<shell_contents_parameters, 8> constexpr shell_contents_qualities{
  shell_contents_parameters{
    .spawn_type{13},
    .colour_1{0},
    .colour_2{6},
  },
  shell_contents_parameters{
    .spawn_type{42},
    .colour_1{6},
    .colour_2{7},
  },
  shell_contents_parameters{
    .spawn_type{11},
    .colour_1{7},
    .colour_2{3},
  },
  shell_contents_parameters{
    .spawn_type{23},
    .colour_1{3},
    .colour_2{9},
  },
  shell_contents_parameters{
    .spawn_type{40},
    .colour_1{9},
    .colour_2{13},
  },
  shell_contents_parameters{
    .spawn_type{41},
    .colour_1{13},
    .colour_2{11},
  },
  shell_contents_parameters{
    .spawn_type{27},
    .colour_1{11},
    .colour_2{0},
  },
  shell_contents_parameters{
    .spawn_type{23},
    .colour_1{2},
    .colour_2{2},
  },
};

struct saucer_parameters {
  uint16_t colour;
  uint16_t flash_colour;
  uint16_t vertical_wave_frequency;
  uint16_t vertical_wave_amplitude;
  uint16_t horizontal_wave_frequency;
  uint16_t horizontal_wave_amplitude;
  uint16_t target_speed;
  uint16_t retreat_exit_threshold;
  uint16_t speed_noise_span;
  uint16_t sideways_exit_threshold;
  uint16_t coast_exit_threshold;
  uint16_t bomb_threshold;
  uint16_t kla_threshold;
  uint16_t weapon_bits;
};

inline std::array<saucer_parameters, 8> constexpr saucer_qualities{
  saucer_parameters{
    .colour{0},
    .flash_colour{15},
    .vertical_wave_frequency{10},
    .vertical_wave_amplitude{0},
    .horizontal_wave_frequency{13},
    .horizontal_wave_amplitude{0},
    .target_speed{200},
    .retreat_exit_threshold{5},
    .speed_noise_span{31},
    .sideways_exit_threshold{5},
    .coast_exit_threshold{5},
    .bomb_threshold{300},
    .kla_threshold{0},
    .weapon_bits{0},
  },
  saucer_parameters{
    .colour{6},
    .flash_colour{14},
    .vertical_wave_frequency{30},
    .vertical_wave_amplitude{1},
    .horizontal_wave_frequency{30},
    .horizontal_wave_amplitude{1},
    .target_speed{200},
    .retreat_exit_threshold{5},
    .speed_noise_span{31},
    .sideways_exit_threshold{30},
    .coast_exit_threshold{30},
    .bomb_threshold{300},
    .kla_threshold{10},
    .weapon_bits{258},
  },
  saucer_parameters{
    .colour{6},
    .flash_colour{15},
    .vertical_wave_frequency{30},
    .vertical_wave_amplitude{5},
    .horizontal_wave_frequency{30},
    .horizontal_wave_amplitude{5},
    .target_speed{200},
    .retreat_exit_threshold{40},
    .speed_noise_span{31},
    .sideways_exit_threshold{5},
    .coast_exit_threshold{10},
    .bomb_threshold{300},
    .kla_threshold{10},
    .weapon_bits{3},
  },
  saucer_parameters{
    .colour{7},
    .flash_colour{9},
    .vertical_wave_frequency{15},
    .vertical_wave_amplitude{20},
    .horizontal_wave_frequency{15},
    .horizontal_wave_amplitude{20},
    .target_speed{200},
    .retreat_exit_threshold{70},
    .speed_noise_span{81},
    .sideways_exit_threshold{70},
    .coast_exit_threshold{5},
    .bomb_threshold{300},
    .kla_threshold{30},
    .weapon_bits{260},
  },
  saucer_parameters{
    .colour{7},
    .flash_colour{14},
    .vertical_wave_frequency{10},
    .vertical_wave_amplitude{40},
    .horizontal_wave_frequency{10},
    .horizontal_wave_amplitude{50},
    .target_speed{200},
    .retreat_exit_threshold{30},
    .speed_noise_span{11},
    .sideways_exit_threshold{15},
    .coast_exit_threshold{100},
    .bomb_threshold{300},
    .kla_threshold{40},
    .weapon_bits{4},
  },
  saucer_parameters{
    .colour{7},
    .flash_colour{15},
    .vertical_wave_frequency{24},
    .vertical_wave_amplitude{80},
    .horizontal_wave_frequency{10},
    .horizontal_wave_amplitude{60},
    .target_speed{200},
    .retreat_exit_threshold{100},
    .speed_noise_span{11},
    .sideways_exit_threshold{50},
    .coast_exit_threshold{30},
    .bomb_threshold{300},
    .kla_threshold{50},
    .weapon_bits{261},
  },
  saucer_parameters{
    .colour{1},
    .flash_colour{14},
    .vertical_wave_frequency{24},
    .vertical_wave_amplitude{160},
    .horizontal_wave_frequency{5},
    .horizontal_wave_amplitude{60},
    .target_speed{200},
    .retreat_exit_threshold{100},
    .speed_noise_span{21},
    .sideways_exit_threshold{100},
    .coast_exit_threshold{100},
    .bomb_threshold{500},
    .kla_threshold{100},
    .weapon_bits{6},
  },
  saucer_parameters{
    .colour{1},
    .flash_colour{15},
    .vertical_wave_frequency{10},
    .vertical_wave_amplitude{150},
    .horizontal_wave_frequency{15},
    .horizontal_wave_amplitude{30},
    .target_speed{200},
    .retreat_exit_threshold{100},
    .speed_noise_span{11},
    .sideways_exit_threshold{200},
    .coast_exit_threshold{200},
    .bomb_threshold{1000},
    .kla_threshold{300},
    .weapon_bits{263},
  },
};

} // namespace corncob::resources

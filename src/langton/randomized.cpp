#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <regex>
#include <random>
#include <unordered_set>

#include "util.hpp"
#include "json.hpp"
#include "langton.hpp"

using json_t = nlohmann::json;

typedef std::uniform_int_distribution<u64> u64_distrib;

static std::string s_words{};
static std::vector<u64> s_words_newline_positions{};

langton::rules_t make_random_rules(
  std::string_view turn_directions,
  std::string &turn_dirs_buffer,
  std::string_view shade_order,
  u64_distrib &dist_rules_len,
  u64_distrib &dist_turn_dir,
  std::mt19937 &num_generator);

void get_rand_name(
  std::string &dest,
  u64 num_words,
  std::mt19937 &num_generator);

langton::state langton::make_randomized_state(
  std::tuple<u64, u64> num_rules_range,
  std::tuple<u64, u64> grid_width_range,
  std::tuple<u64, u64> grid_height_range,
  std::string_view turn_directions,
  std::string_view shade_order,
  std::string_view ant_orientations
  // std::string_view name_mode
) {
  std::random_device rand_device;
  std::mt19937 rand_num_gener(rand_device());

  assert(std::get<0>(num_rules_range) >= 2);
  assert(std::get<1>(num_rules_range) <= 256);
  assert(shade_order == "asc" || shade_order == "desc" || shade_order == "rand");

  u64_distrib distrib_rules_len(std::get<0>(num_rules_range), std::get<1>(num_rules_range));
  u64_distrib distrib_grid_width(std::get<0>(grid_width_range), std::get<1>(grid_width_range));
  u64_distrib distrib_grid_height(std::get<0>(grid_height_range), std::get<1>(grid_height_range));
  u64_distrib distrib_turn_dir(u64(0), turn_directions.length() - 1);
  u64_distrib distrib_ant_orient(u64(0), ant_orientations.length() - 1);

  // only used if name_mode is randwords
  std::string rand_fname{};

  std::string turn_dirs_buffer(256 + 1, '\0');
  langton::rules_t rand_rules{};
  std::unordered_set<std::string> names{};

  langton::orientation::value_type const rand_ant_orient = [&] {
    u64 const rand_idx = distrib_ant_orient(rand_num_gener);

    char const rand_ant_orient_str[] {
      static_cast<char>(toupper(ant_orientations[rand_idx])),
      '\0'
    };

    return langton::orientation::from_cstr(rand_ant_orient_str);
  }();

  rand_rules = make_random_rules(
    turn_directions,
    turn_dirs_buffer,
    shade_order,
    distrib_rules_len,
    distrib_turn_dir,
    rand_num_gener);

  auto rules_err = langton::validate_rules(rand_rules);
  assert(rules_err.empty());

  u64 rand_gw = distrib_grid_width(rand_num_gener);
  u64 rand_gh = distrib_grid_height(rand_num_gener);

  // u64 rand_ac = distrib_grid_width(rand_num_gener);
  // u64 rand_ar = distrib_grid_height(rand_num_gener);
  i32 ac = i32(rand_gw / 2);
  i32 ar = i32(rand_gh / 2);

  u8 *grid = new u8[rand_gw * rand_gh];

  // Find a rule which has a valid turn direction
  std::vector<u8> valid_starting_grid_colors = {};
  for (u64 i = 0; i < rand_rules.size(); ++i) {
    auto const &rule = rand_rules[i];
    if (rule.turn_dir != turn_direction::NIL) {
      valid_starting_grid_colors.push_back(u8(i));
    }
  }

  std::sort(valid_starting_grid_colors.begin(), valid_starting_grid_colors.end());
  u64_distrib distrib_grid_starting_color_idx(0, valid_starting_grid_colors.size() - 1);
  u8 grid_starting_color = u8(distrib_grid_starting_color_idx(rand_num_gener));

  std::fill_n(grid, rand_gw * rand_gh, grid_starting_color);

  langton::state state = {
    .start_generation = 0,
    .generation = 0,
    .grid = grid,
    .grid_width = i32(rand_gw),
    .grid_height = i32(rand_gh),
    .ant_col = ac,
    .ant_row = ar,
    .ant_orientation = rand_ant_orient,
    .last_step_res = langton::step_result::NIL,
    .maxval = deduce_maxval_from_rules(rand_rules),
    .rules = rand_rules,
    .rules_reversed = langton::reverse_rules(rand_rules),
  };

  return state;
}

langton::rules_t make_random_rules(
  std::string_view turn_directions,
  std::string &turn_dirs_buffer,
  std::string_view shade_order,
  u64_distrib &dist_rules_len,
  u64_distrib &dist_turn_dir,
  std::mt19937 &num_generator)
{
  langton::rules_t rules{};

  u64 const rules_len = dist_rules_len(num_generator);
  assert(rules_len >= 2);
  assert(rules_len <= 256);

  // fill `turn_dirs_buffer` with `rules_len` random turn directions
  turn_dirs_buffer.clear();
  for (u64 i = 0; i < rules_len; ++i) {
    u64 const rand_idx = dist_turn_dir(num_generator);
    char const rand_turn_dir = static_cast<char>(toupper(turn_directions[rand_idx]));
    turn_dirs_buffer += rand_turn_dir;
  }

  u64 const last_rule_idx = rules_len - 1;

  auto const set_rule = [&](
    u64 const shade,
    u64 const replacement_shade)
  {
    rules[shade].turn_dir = langton::turn_direction::from_char(turn_dirs_buffer[shade]);
    rules[shade].replacement_shade = static_cast<u8>(replacement_shade);
  };

  if (shade_order == "asc") {
    // 0->1
    // 1->2  0->1->2->3
    // 2->3  ^--------'
    // 3->0

    for (u64 i = 0; i < last_rule_idx; ++i)
      set_rule(i, i + 1);
    set_rule(last_rule_idx, 0);

  } else if (shade_order == "desc") {
    // 0->3
    // 1->0  3->2->1->0
    // 2->1  ^--------'
    // 3->2

    for (u64 i = last_rule_idx; i > 0; --i)
      set_rule(i, i - 1);
    set_rule(0, last_rule_idx);

  } else { // rand
    std::vector<u8> remaining_shades{};
    remaining_shades.reserve(rules_len);

    for (u64 shade = 0; shade < rules_len; ++shade)
      remaining_shades.push_back(static_cast<u8>(shade));

    std::vector<u8> chain{};
    chain.reserve(rules_len);

    // [0]->[1]->[2]->[3] ...indices
    //  ^--------------'

    for (u64 i = 0; i < rules_len; ++i) {
      u64_distrib remaining_idx_dist(0, remaining_shades.size() - 1);
      u64 const rand_shade_idx = remaining_idx_dist(num_generator);
      u8 const rand_shade = remaining_shades[rand_shade_idx];
      {
        // remove chosen shade by swapping it with the last shade, then popping
        u8 const temp = remaining_shades.back();
        remaining_shades.back() = rand_shade;
        remaining_shades[rand_shade_idx] = temp;
        remaining_shades.pop_back();
      }
      chain.push_back(rand_shade);
    }

    for (auto shade_it = chain.begin(); shade_it < chain.end() - 1; ++shade_it)
      set_rule(*shade_it, *(shade_it + 1));
    set_rule(chain.back(), chain.front());
  }

  return rules;
}

void get_rand_name(
  std::string &dest,
  u64 num_words,
  std::mt19937 &num_generator)
{
  dest.clear();

  auto const pick_rand_word = [&]() -> std::string_view
  {
    u64_distrib distrib(0, s_words_newline_positions.size() - 2);
    // size-2 because we don't want to pick the very last newline
    // since we create the view going forwards from the chosen newline
    // to the next newline.

    u64 const rand_newline_idx = distrib(num_generator);
    u64 const next_newline_idx = rand_newline_idx + 1;

    u64 const start_pos = s_words_newline_positions[rand_newline_idx] + 1;
    u64 const count = s_words_newline_positions[next_newline_idx] - start_pos;

    return { s_words.c_str() + start_pos, count };
  };

  for (u64 i = 0; i < num_words - 1; ++i) {
    dest.append(pick_rand_word());
    dest += '_';
  }
  dest.append(pick_rand_word());
}

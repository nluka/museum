#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <array>
#include <cinttypes>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>
#include <variant>
#include <ostream>

// #include <boost/program_options.hpp>

#include "pgm8.hpp"
#include "primitives.hpp"
#include "util.hpp"

namespace langton
{
  typedef std::vector< std::tuple<char const * /*name*/, char const * /*json string*/> > examples_t;
  examples_t make_langton_examples();

  typedef u8 cluster_t;
  cluster_t const NO_CLUSTER = 0;
  cluster_t next_cluster(std::vector<std::filesystem::path> const &clusters);

  // not using an enum class because I want to do arithmetic with
  // turn_direction values without casting.
  namespace orientation
  {
    typedef i8 value_type;

    value_type from_cstr(char const *);
    char const *to_cstr(value_type);

    value_type const OVERFLOW_COUNTER_CLOCKWISE = 0;
    value_type const NORTH = 1;
    value_type const EAST = 2;
    value_type const SOUTH = 3;
    value_type const WEST = 4;
    value_type const OVERFLOW_CLOCKWISE = 5;
  }

  // not using an enum class because I want to do arithmetic with
  // orientation values without casting.
  namespace turn_direction
  {
    typedef i8 value_type;

    value_type from_char(char);
    char const *to_cstr(value_type);

    value_type const NIL = -2;
    value_type const LEFT = -1;
    value_type const NO_CHANGE = 0;
    value_type const RIGHT = 1;
  }

  // not using an enum class because the other 2 enumerated types are done with
  // namespaces, so let's stay consistent.
  namespace step_result
  {
    typedef i8 value_type;

    char const *to_cstr(value_type);

    value_type const NIL = -1;
    value_type const SUCCESS = 0;
    value_type const HIT_EDGE = 1;
  }

  struct rule
  {
    u8 replacement_shade = 0;
    turn_direction::value_type turn_dir = turn_direction::NIL;

    b8 operator!=(rule const &) const noexcept;
    friend std::ostream &operator<<(std::ostream &, rule const &);
  };

  // make sure there's no padding
  static_assert(sizeof(rule) == 2);

  typedef std::array<rule, 256> rules_t;

  struct state
  {
    u64 start_generation;
    u64 generation;
    u8 *grid;
    i32 grid_width;
    i32 grid_height;
    i32 ant_col;
    i32 ant_row;
    orientation::value_type ant_orientation;
    step_result::value_type last_step_res;
    u8 maxval;
    rules_t rules;
    rules_t rules_reversed;

    b8 can_step_forward(u64 generation_limit = 0) const noexcept;
    u64 num_pixels() const noexcept;
    u64 generations_completed() const noexcept;
  };

  rules_t reverse_rules(rules_t const &rules);

  std::string validate_rules(rules_t const &rules);

  state parse_state(
    std::string const &json_str,
    std::filesystem::path const &dir,
    util::errors_t &errors);

  state make_randomized_state(
    std::tuple<u64, u64> num_rules_range,
    std::tuple<u64, u64> grid_width_range,
    std::tuple<u64, u64> grid_height_range,
    std::string_view turn_directions,
    std::string_view shade_order,
    std::string_view ant_orientations);

  step_result::value_type attempt_step_forward(state &state);

  step_result::value_type attempt_step_backward(state &state);

  u8 deduce_maxval_from_rules(rules_t const &rules);

  struct save_state_result
  {
    b8 state_write_success;
    b8 image_write_success;
  };

  save_state_result save_state(
    state const &state,
    const char *name,
    std::filesystem::path const &dir,
    pgm8::format img_fmt,
    b8 image_only);

  bool print_state_json(
    std::ostream &os,
    std::string const &file_path,
    std::string const &grid_state,
    b8 grid_state_is_path,
    u64 generation,
    i32 grid_width,
    i32 grid_height,
    i32 ant_col,
    i32 ant_row,
    step_result::value_type last_step_res,
    orientation::value_type ant_orientation,
    u64 maxval_digits,
    rules_t const &rules);

  std::string extract_name_from_json_state_path(std::string const &);
}

#endif // SIMULATION_HPP

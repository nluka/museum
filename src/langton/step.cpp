#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <atomic>
#include <mutex>

#include "langton.hpp"

namespace fs = std::filesystem;

static std::atomic<u64> s_num_consecutive_save_fails = 0;
static std::mutex s_death_mutex{};

// Removes duplicate elements from a sorted vector.
template <typename ElemTy>
std::vector<ElemTy> remove_duplicates_sorted(std::vector<ElemTy> const &input)
{
  if (input.size() < 2) {
    return input;
  }

  std::vector<ElemTy> output(input.size());

  // check the first element individually
  if (input[0] != input[1]) {
    output.push_back(input[0]);
  }

  for (u64 i = 1; i < input.size() - 1; ++i) {
    if (input[i] != input[i - 1] && input[i] != input[i + 1]) {
      output.push_back(input[i]);
    }
  }

  // check the last item individually
  if (input[input.size() - 1] != input[input.size() - 2]) {
    output.push_back(input[input.size() - 1]);
  }

  return output;
}

// Finds the index of the smallest value in an array.
template <typename Ty>
u64 idx_of_smallest(Ty const *const values, u64 const num_values)
{
  assert(num_values > 0);
  assert(values != nullptr);

  u64 min_idx = 0;
  for (u64 i = 1; i < num_values; ++i) {
    if (values[i] < values[min_idx]) {
      min_idx = i;
    }
  }

  return min_idx;
}

langton::step_result::value_type langton::attempt_step_forward(
  langton::state &state)
{
  u64 const curr_cell_idx = (u64(state.ant_row) * u64(state.grid_width)) + u64(state.ant_col);
  u8 const curr_cell_shade = state.grid[curr_cell_idx];
  auto const &curr_cell_rule = state.rules[curr_cell_shade];

  assert(curr_cell_rule.turn_dir != turn_direction::NIL);

  // turn
  state.ant_orientation = state.ant_orientation + curr_cell_rule.turn_dir;
  if (state.ant_orientation == orientation::OVERFLOW_COUNTER_CLOCKWISE)
    state.ant_orientation = orientation::WEST;
  else if (state.ant_orientation == orientation::OVERFLOW_CLOCKWISE)
    state.ant_orientation = orientation::NORTH;

  // update current cell shade
  state.grid[curr_cell_idx] = curr_cell_rule.replacement_shade;

  i32 next_col, next_row;
#if 0
  if (state.ant_orientation == orientation::NORTH) {
    next_col = state.ant_col;
    next_row = state.ant_row - 1;
  } else if (state.ant_orientation == orientation::EAST) {
    next_col = state.ant_col + 1;
    next_row = state.ant_row;
  } else if (state.ant_orientation == orientation::SOUTH) {
    next_row = state.ant_row + 1;
    next_col = state.ant_col;
  } else { // west
    next_col = state.ant_col - 1;
    next_row = state.ant_row;
  }
#else
  if (state.ant_orientation == orientation::EAST)
    next_col = state.ant_col + 1;
  else if (state.ant_orientation == orientation::WEST)
    next_col = state.ant_col - 1;
  else
    next_col = state.ant_col;

  if (state.ant_orientation == orientation::NORTH)
    next_row = state.ant_row - 1;
  else if (state.ant_orientation == orientation::SOUTH)
    next_row = state.ant_row + 1;
  else
    next_row = state.ant_row;
#endif

  if (
    util::in_range_incl_excl(next_col, 0, state.grid_width) &&
    util::in_range_incl_excl(next_row, 0, state.grid_height)) [[likely]] {
    state.ant_col = next_col;
    state.ant_row = next_row;
    return step_result::SUCCESS;
  } else [[unlikely]] {
    return step_result::HIT_EDGE;
  }
}

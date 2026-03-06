#include <cassert>

#include "langton.hpp"

u8 langton::next_cluster(std::vector<std::filesystem::path> const &clusters)
{
  // [0]   is the number of times "cluster1" occurred,
  // [254] is the number of times "cluster255" occurred
  std::array<u64, 255> cluster_num_occurences{};

  for (auto const &cluster : clusters) {
    char const *const cluster_num_cstr = std::strpbrk(cluster.string().c_str(), "0123456789");

    if (cluster_num_cstr != nullptr) {
      u64 const cluster_num = std::stoull(cluster_num_cstr);

      if (cluster_num == 0 || cluster_num > std::numeric_limits<cluster_t>::max())
        continue;

      ++cluster_num_occurences[cluster_num - 1];
    }
  }

  for (u64 i = 0; i < cluster_num_occurences.size(); ++i) {
    u64 const cluster_occurences = cluster_num_occurences[i];

    if (cluster_occurences == 0)
      return static_cast<cluster_t>(i + 1);
  }

  // all clusters [1, 255] exist, no cluster opportunity available
  return NO_CLUSTER;
}

char const *langton::orientation::to_cstr(orientation::value_type const orient)
{
  switch (orient) {
    case orientation::NORTH: return "N";
    case orientation::EAST: return "E";
    case orientation::SOUTH: return "S";
    case orientation::WEST: return "W";

    default:
      throw std::runtime_error("orientation::to_string failed - bad orient");
  }
}

langton::orientation::value_type langton::orientation::from_cstr(char const *str)
{
  if (strcmp(str, "N") == 0) return orientation::NORTH;
  if (strcmp(str, "E") == 0) return orientation::EAST;
  if (strcmp(str, "S") == 0) return orientation::SOUTH;
  if (strcmp(str, "W") == 0) return orientation::WEST;
  throw std::runtime_error("orientation::from_cstr failed - bad str");
}

char const *langton::turn_direction::to_cstr(turn_direction::value_type const turn_dir)
{
  switch (turn_dir) {
    case turn_direction::LEFT: return "L";
    case turn_direction::NO_CHANGE: return "N";
    case turn_direction::RIGHT: return "R";

    default:
      throw std::runtime_error("turn_direction::to_string failed - bad turn_dir");
  }
}

langton::turn_direction::value_type langton::turn_direction::from_char(char const ch)
{
  switch (ch) {
    case 'L': return turn_direction::LEFT;
    case 'N': return turn_direction::NO_CHANGE;
    case 'R': return turn_direction::RIGHT;

    default:
      throw std::runtime_error("turn_direction::from_char failed - bad ch");
  }
}

char const *langton::step_result::to_cstr(step_result::value_type const step_res)
{
  switch (step_res) {
    case step_result::NIL: return "nil";
    case step_result::SUCCESS: return "success";
    case step_result::HIT_EDGE: return "hit_edge";

    default:
      throw std::runtime_error("step_result::to_string failed - bad step_res");
  }
}

b8 langton::rule::operator!=(langton::rule const &other) const noexcept
{
  return this->replacement_shade != other.replacement_shade || this->turn_dir != other.turn_dir;
}

namespace langton
{
  // define ostream insertion operator for ntest
  std::ostream &operator<<(std::ostream &os, langton::rule const &rule)
  {
    os << '(' << static_cast<i32>(rule.replacement_shade)
       << ", " << static_cast<i32>(rule.turn_dir) << ')';
    return os;
  }
}

b8 langton::state::can_step_forward(u64 const generation_limit) const noexcept
{
  return
    (last_step_res <= langton::step_result::SUCCESS)
    && (generation < generation_limit);
}

u8 langton::deduce_maxval_from_rules(langton::rules_t const &rules)
{
  for (u8 shade = 255; shade > 0; --shade)
    if (rules[shade].turn_dir != langton::turn_direction::NIL)
      return shade;
  return 0;
}

u64 langton::state::num_pixels() const noexcept
{
  assert(this->grid_width >= 0);
  assert(this->grid_height >= 0);

  return u64(this->grid_width) * u64(this->grid_height);
}

u64 langton::state::generations_completed() const noexcept
{
  assert(generation >= start_generation);

  return generation - start_generation;
}

std::string langton::extract_name_from_json_state_path(std::string const &json_path)
{
  assert(!json_path.empty());

  std::filesystem::path p(json_path);
  p = p.filename();
  p.replace_extension("");

  std::string filename_no_extension(p.string());

  size_t const opening_paren_pos = filename_no_extension.find_first_of('(');

  if (opening_paren_pos == std::string::npos)
    return filename_no_extension;
  else
    return filename_no_extension.erase(opening_paren_pos);
}

std::string langton::validate_rules(langton::rules_t const &rules)
{
  std::array<u16, 256> shade_occurences{};
  u64 num_defined_rules = 0;

  for (u64 i = 0; i < rules.size(); ++i) {
    auto const &r = rules[i];
    b8 const is_rule_used = r.turn_dir != langton::turn_direction::NIL;
    if (is_rule_used) {
      ++num_defined_rules;
      ++shade_occurences[i];
      ++shade_occurences[r.replacement_shade];
    }
  }

  if (num_defined_rules < 2) {
    return "bad rules, fewer than 2 defined";
  }

  u64 num_non_zero_occurences = 0, num_non_two_occurences = 0;
  for (auto const occurencesOfShade : shade_occurences) {
    if (occurencesOfShade != 0) {
      ++num_non_zero_occurences;
      if (occurencesOfShade != 2)
        ++num_non_two_occurences;
    }
  }

  if (num_non_zero_occurences < 2 || num_non_two_occurences > 0) {
    return "bad rules, don't form a closed chain";
  }

  return "";
}

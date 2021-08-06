/*
 * Copyright (c) 2021, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <iostream>
#include <limits>
#include <random>
#include <string>

enum class dist_type { GAUSSIAN, GEOMETRIC, UNIFORM, UNIQUE, SAME };

#if defined(NVBENCH_MODULE)
#include <nvbench/nvbench.cuh>

NVBENCH_DECLARE_ENUM_TYPE_STRINGS(
  // Enum type:
  dist_type,
  // Callable to generate input strings:
  // Short identifier used for tables, command-line args, etc.
  // Used when context is available to figure out the enum type.
  [](dist_type d) {
    switch (d) {
      case dist_type::GAUSSIAN: return "GAUSSIAN";
      case dist_type::GEOMETRIC: return "GEOMETRIC";
      case dist_type::UNIFORM: return "UNIFORM";
      case dist_type::UNIQUE: return "UNIQUE";
      case dist_type::SAME: return "SAME";
      default: return "ERROR";
    }
  },
  // Callable to generate descriptions:
  // If non-empty, these are used in `--list` to describe values.
  // Used when context may not be available to figure out the type from the
  // input string.
  // Just use `[](auto) { return std::string{}; }` if you don't want these.
  [](auto) { return std::string{}; })
#endif

template <typename Key, typename OutputIt>
static bool generate_keys(dist_type dist,
                          OutputIt output_begin,
                          OutputIt output_end,
                          std::size_t multiplicity = 8)
{
  auto const num_keys = std::distance(output_begin, output_end);

  std::random_device rd;
  std::mt19937 gen{rd()};

  switch (dist) {
    case dist_type::GAUSSIAN: {
      auto const mean = static_cast<double>(num_keys / 2);
      auto const dev  = static_cast<double>(num_keys / 5);

      std::normal_distribution<> distribution{mean, dev};

      for (auto i = 0; i < num_keys; ++i) {
        auto k = distribution(gen);
        while (k >= num_keys) {
          k = distribution(gen);
        }
        output_begin[i] = k;
      }
      break;
    }
    case dist_type::GEOMETRIC: {
      auto const max   = std::numeric_limits<int32_t>::max();
      auto const coeff = static_cast<double>(num_keys) / static_cast<double>(max);
      // Random sampling in range [0, INT32_MAX]
      std::geometric_distribution<Key> distribution{1e-9};

      for (auto i = 0; i < num_keys; ++i) {
        output_begin[i] = distribution(gen) * coeff;
      }
      break;
    }
    case dist_type::UNIFORM: {
      std::uniform_int_distribution<Key> distribution{1, static_cast<Key>(num_keys / multiplicity)};

      for (auto i = 0; i < num_keys; ++i) {
        output_begin[i] = distribution(gen);
      }
      break;
    }
    case dist_type::UNIQUE: {
      // 3 because some HT implementations use 0, 1 as sentinels
      for (auto i = 2; i < num_keys + 2; ++i) {
        output_begin[i] = i;
      }
      std::random_shuffle(output_begin, output_end);
      break;
    }
    case dist_type::SAME: {
      std::fill(output_begin, output_end, Key(42));
      break;
    }
    default: {
      return false;
    }
  }  // switch

  return true;
}

template <typename Key, typename OutputIt>
static bool generate_keys(std::string const& dist,
                          OutputIt output_begin,
                          OutputIt output_end,
                          std::size_t multiplicity = 8)
{
  dist_type enum_value{};

  if (dist == "GAUSSIAN") {
    enum_value = dist_type::GAUSSIAN;
  } else if (dist == "GEOMETRIC") {
    enum_value = dist_type::GEOMETRIC;
  } else if (dist == "UNIFORM") {
    enum_value = dist_type::UNIFORM;
  } else if (dist == "UNIQUE") {
    enum_value = dist_type::UNIQUE;
  } else if (dist == "SAME") {
    enum_value = dist_type::SAME;
  } else {
    return false;
  }

  return generate_keys<Key>(enum_value, output_begin, output_end, multiplicity);
}

template <typename Key, typename OutputIt>
static void generate_prob_keys(double const matching_rate,
                               OutputIt output_begin,
                               OutputIt output_end)
{
  auto const num_keys = std::distance(output_begin, output_end);
  auto const max      = std::numeric_limits<Key>::max() - 2;

  std::random_device rd;
  std::mt19937 gen{rd()};

  std::uniform_real_distribution<double> rate_dist(0.0, 1.0);
  std::uniform_int_distribution<Key> non_match_dist{static_cast<Key>(num_keys + 2), max};

  for (auto i = 0; i < num_keys; ++i) {
    auto const tmp_rate = rate_dist(gen);

    if (tmp_rate > matching_rate) { output_begin[i] = non_match_dist(gen); }
  }

  std::random_shuffle(output_begin, output_end);
}
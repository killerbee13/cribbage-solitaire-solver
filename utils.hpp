/* *****************************************************************************
 * cribbage-solitaire-solver
 * Copyright (c) 2026 killerbee
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ****************************************************************************/
#ifndef UTILS_HPP
#define UTILS_HPP

#include "kblib/iterators.h"
#include "kblib/stats.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <csignal>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

using std::begin, std::cbegin, std::rbegin, std::crbegin, std::end, std::cend,
    std::rend, std::crend;
using std::exchange, std::all_of, std::accumulate, kblib::min, kblib::max,
    kblib::range, kblib::etoi;
using std::istream, std::ostream, std::cin, std::cout, std::cerr,
    std::exception, std::invalid_argument;
using std::size_t, std::uint8_t;
using std::string, std::string_view, std::vector, std::array,
    std::unordered_map, std::map, std::pair, std::span;
using namespace std::literals;
namespace stdr = std::ranges;
namespace stdv = std::ranges::views;

inline std::atomic<std::sig_atomic_t> stop_requested;
inline std::atomic<std::sig_atomic_t> info_requested;

#endif // UTILS_HPP

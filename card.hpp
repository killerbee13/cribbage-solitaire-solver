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
#ifndef CARD_HPP
#define CARD_HPP

#include "inplace_vector.hpp"
#include "utils.hpp"

#include "kblib/containers.h"
#include "kblib/stringops.h"

#include <csignal>
#include <ostream>

enum card : uint8_t {
	card_null = 0,
	ace = 1,
	ten = 10,
	jack = 11,
	queen = 12,
	king = 13
};
constexpr inline auto ranks = 13u;
constexpr inline auto suits = 4u;
constexpr inline auto deck_size = ranks * suits;

constexpr inline auto tableau_width = 4u;
constexpr inline auto tableau_depth = 13u;

constexpr inline auto value(card c) -> int {
	if (c >= card::jack) {
		return 10;
	} else {
		return static_cast<int>(c);
	}
}
inline auto card_from_label(char s) -> card {
	static const unordered_map<char, card> map{
	    {'A', ace},  {'0', ten},   {'T', ten}, {'X', ten},
	    {'J', jack}, {'Q', queen}, {'K', king}};
	if (s > '0' and s <= '9') {
		return static_cast<card>(s - '0');
	} else {
		return kblib::get_or(map, kblib::toupper(s), card::card_null);
	}
}
inline auto label_for_card(card c) -> char {
	static const unordered_map<card, char> map{{card_null, '-'}, {ace, 'A'},
	                                           {ten, 'X'},       {jack, 'J'},
	                                           {queen, 'Q'},     {king, 'K'}};
	if (c > ace and c < ten) {
		return static_cast<char>(c + '0');
	} else {
		return kblib::get_or(map, c, '-');
	}
}
inline ostream& operator<<(ostream& os, card c) {
	return os << label_for_card(c);
}

using card_stack = inplace_vector<card, tableau_depth>;

#endif // CARD_HPP

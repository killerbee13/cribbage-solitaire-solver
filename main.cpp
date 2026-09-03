/* *****************************************************************************
 * %{QMAKE_PROJECT_NAME}
 * Copyright (c) %YEAR% killerbee
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
#include "kblib/iterators.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <iostream>
#include <numeric>
#include <ranges>
#include <vector>

enum card : std::uint8_t {
	null = 0,
	ace = 1,
	jack = 11,
	queen = 12,
	king = 13
};
std::size_t ranks = 13;

int score(card c) {
	if (c >= card::jack) {
		return 10;
	} else {
		return static_cast<int>(c);
	}
}

bool is_consecutive(std::ranges::forward_range auto&& range) {
	if (std::empty(range)) {
		return true;
	} else {
		return std::ranges::equal(range,
		                          kblib::range(*begin(range), *end(range)));
	}
}

struct game {
	std::array<std::vector<card>, 4> tableau;
	std::vector<card> stack;

	int score() const {
		if (stack.empty()) {
			return 0;
		}
		const auto t = total();
		auto s = 0;
		if (stack.front() == card::jack) {
			s += 2;
		}
		if (t == 15) {
			s += 2;
		} else if (t == 31) {
			s += 2;
		}
		for (std::pair<card, int> same{}; auto c : std::views::reverse(stack)) {
			if (c == same.first) {
				++same.second;
			} else {
				same = {c, 1};
			}
			const std::array<int, 4> same_scores{0, 2, 6, 12};
			s += same_scores[same.second];
		}
		for (auto size : kblib::range(std::max(7uz, stack.size()), 2uz, -1uz)) {
			std::vector<card> last(end(stack) - size, end(stack));
			std::sort(begin(last), end(last));
			if (is_consecutive(last)) {
				s += size;
				break;
			}
		}
		return s;
	}
	int total() const {
		return std::accumulate(begin(stack), end(stack), 0,
		                       [](int a, card c) { return a + ::score(c); });
	}
	std::array<card, 4> top() const;
	card top(std::size_t i) const {
		assert(i < tableau.size());
		if (not tableau[i].empty()) {
			return tableau[i].back();
		} else {
			return card{};
		}
	}
	card pop(std::size_t i) {
		assert(i < tableau.size() and not tableau[i].empty());
		auto card = tableau[i].back();
		tableau[i].pop_back();
		return card;
	}
};

int main() {}

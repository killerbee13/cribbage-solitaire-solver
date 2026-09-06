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
#include "kblib/containers.h"
#include "kblib/io.h"
#include "kblib/iterators.h"
#include "kblib/stringops.h"
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
	ten = 10,
	jack = 11,
	queen = 12,
	king = 13
};
std::size_t ranks = 13;

int value(card c) {
	if (c >= card::jack) {
		return 10;
	} else {
		return static_cast<int>(c);
	}
}
card card_from_label(char s) {
	static const std::unordered_map<char, card> map{{'A', ace},   {'0', ten},
	                                                {'X', ten},   {'J', jack},
	                                                {'Q', queen}, {'K', king}};
	if (s >= '0' and s <= '9') {
		return static_cast<card>(s - '0');
	} else {
		return kblib::get_or(map, kblib::toupper(s), card::null);
	}
}
char label_for_card(card c) {
	static const std::unordered_map<card, char> map{{null, '-'},  {ace, 'A'},
	                                                {ten, 'X'},   {jack, 'J'},
	                                                {queen, 'Q'}, {king, 'K'}};
	if (c > ace and c < ten) {
		return static_cast<char>(c + '0');
	} else {
		return kblib::get_or(map, c, '-');
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
	std::array<std::vector<card>, 4> tableau_{};
	std::vector<card> stack_{};
	int score_{};

	card pop(size_t i) {
		assert(i < tableau_.size() and not tableau_[i].empty());
		return kblib::pop(tableau_[i]);
	}
	card top(size_t i) const {
		assert(i < tableau_.size());
		if (not tableau_[i].empty()) {
			return tableau_[i].back();
		} else {
			return card::null;
		}
	}
	std::array<card, 4> top() const { return {top(0), top(1), top(2), top(3)}; }
	int total() const {
		return std::accumulate(begin(stack_), end(stack_), 0,
		                       [](int a, card c) { return a + value(c); });
	}

	bool append(size_t i) {
		assert(i < tableau_.size());
		assert(not tableau_[i].empty());
		if (total() + value(top(i)) > 31) {
			return false;
		}
		return append(pop(i));
	}
	bool append(card c) {
		const auto t = total() + value(c);
		// stack total may not exceed 31
		if (t > 31) {
			return false;
		}
		// first card in stack is a jack = +2 pts
		if (stack_.empty() and c == card::jack) {
			score_ += 2;
		}
		// stack total is exactly 15 = +2 pts
		if (t == 15) {
			score_ += 2;
			// stack total is exactly 31 = +2 pts
		} else if (t == 31) {
			score_ += 2;
		}

		// set of 2, 3, or 4 of the same card = +2/+6/+12 pts
		auto same_count = std::find_if(rbegin(stack_), rend(stack_),
		                               [c](card x) { return x != c; })
		                  - rbegin(stack_);
		const std::array<int, 4> same_scores{0, 2, 6, 12};
		score_ += same_scores[same_count];
		stack_.push_back(c);

		// run of 3 to 7 cards, in any order = +3 to +7 pts
		for (auto size : kblib::range(std::max(7uz, stack_.size()), 2uz, -1uz)) {
			std::vector<card> last(end(stack_) - size, end(stack_));
			std::sort(begin(last), end(last));
			if (is_consecutive(last)) {
				score_ += size;
				break;
			}
		}
		return true;
	}

	int get_score() const {
		if (stack_.empty()) {
			return 0;
		}
		const auto t = total();
		auto s = 0;
		if (stack_.front() == card::jack) {
			s += 2;
		}
		if (t == 15) {
			s += 2;
		} else if (t == 31) {
			s += 2;
		}
		for (std::pair<card, int> same{}; auto c : std::views::reverse(stack_)) {
			if (c == same.first) {
				++same.second;
			} else {
				same = {c, 1};
			}
			const std::array<int, 4> same_scores{0, 2, 6, 12};
			s += same_scores[same.second];
		}
		for (auto size : kblib::range(std::max(7uz, stack_.size()), 2uz, -1uz)) {
			std::vector<card> last(end(stack_) - size, end(stack_));
			std::sort(begin(last), end(last));
			if (is_consecutive(last)) {
				s += size;
				break;
			}
		}
		return s;
	}

	friend std::istream& operator>>(std::istream& is, game& g) {
		for (auto& col : g.tableau_) {
			auto line = kblib::getline(is);
			for (auto c : line) {
				col.push_back(card_from_label(c));
			}
		}
		return is;
	}
	friend game read_deal(std::istream& is) {
		game g;
		is >> g;
		return g;
	}
	friend std::ostream& operator<<(std::ostream& os, const game& g) {
		for (auto col : g.tableau_) {
			for (auto c : col) {
				os << label_for_card(c);
			}
			os << ',';
		}
		for (auto c : g.stack_) {
			os << label_for_card(c);
		}
		return os;
	}
	game() = default;
	game(std::string_view repr) {
		auto sections = kblib::split_dsv(repr, ',');
		if (sections.size() < 4 or sections.size() > 5) {
			throw std::invalid_argument("games must have 4 or 5 sections");
		}
		for (auto i : kblib::range(4u)) {
			for (auto c : sections[i]) {
				tableau_[i].push_back(card_from_label(c));
			}
		}
		if (sections.size() == 5u) {
			for (auto c : sections[4u]) {
				append(card_from_label(c));
			}
		}
	}
};
game read_deal(std::istream& is);

int main(int argc, char** argv) {
	if (argc > 1) {
		for (std::string_view sv : kblib::indirect(&argv[1], &argv[argc])) {
			auto g = game(sv);
			std::cout << g << '\n' << g.score_ << '\n';
			while (g.append(0u)) {
			}
			std::cout << g << '\n' << g.score_ << '\n';
		}
	} else {
		auto g = read_deal(std::cin);
		std::cout << g << '\n' << g.score_ << '\n';
		while (g.append(0u)) {
		}
		std::cout << g << '\n' << g.score_ << '\n';
	}
}

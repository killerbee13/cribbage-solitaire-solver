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
#include "kblib/random.h"
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
constexpr std::size_t ranks = 13;
constexpr std::size_t suits = 4;
constexpr std::size_t deck_size = ranks * suits;

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
	static constexpr auto tableau_width = 4u;
	static constexpr auto tableau_depth = 13u;
	static constexpr auto total_limit = 31;
	static constexpr auto score_target = 63;

	std::array<std::vector<card>, tableau_width> tableau_{};
	std::vector<card> stack_{};
	int score_{};
	int stack_score_{};

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
	std::array<card, tableau_width> top() const {
		return {top(0), top(1), top(2), top(3)};
	}
	int total() const {
		return std::accumulate(begin(stack_), end(stack_), 0,
		                       [](int a, card c) { return a + value(c); });
	}

	// returns false when?
	bool append(size_t i) {
		assert(i < tableau_.size());
		assert(not tableau_[i].empty());
		if (total() + value(top(i)) > total_limit) {
			for (auto c : top()) {
				if (total() + value(c) <= total_limit) {
					return false;
				}
			}
			return false;
		}
		return append(pop(i));
	}
	bool append(card c) {
		const auto t = total() + value(c);
		// stack total may not exceed 31
		if (t > total_limit) {
			return false;
		}
		// first card in stack is a jack = +2 pts
		if (stack_.empty() and c == card::jack) {
			stack_score_ += 2;
		}
		// stack total is exactly 15 = +2 pts
		if (t == 15) {
			stack_score_ += 2;
			// stack total is exactly 31 = +2 pts
		} else if (t == 31) {
			stack_score_ += 2;
		}

		// set of 2, 3, or 4 of the same card = +2/+6/+12 pts
		auto same_count = std::find_if(rbegin(stack_), rend(stack_),
		                               [c](card x) { return x != c; })
		                  - rbegin(stack_);
		const std::array same_scores{0, 2, 6, 12};
		stack_score_ += same_scores[same_count];
		stack_.push_back(c);

		// run of 3 to 7 cards, in any order = +3 to +7 pts
		for (auto size : kblib::range(std::max(7uz, stack_.size()), 2uz, -1uz)) {
			std::vector<card> last(end(stack_) - size, end(stack_));
			std::sort(begin(last), end(last));
			if (is_consecutive(last)) {
				stack_score_ += size;
				break;
			}
		}
		return true;
	}

	int get_stack_score() const {
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
			const std::array same_scores{0, 2, 6, 12};
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

	void assign(std::string_view repr) {
		auto sections = kblib::split_dsv(repr, ',');
		if (sections.size() < tableau_width
		    or sections.size() > (tableau_width + 1)) {
			throw std::invalid_argument("games must have 4 or 5 sections");
		}
		score_ = 0;
		stack_score_ = 0;
		stack_.clear();
		for (auto i : kblib::range(tableau_width)) {
			tableau_[i].clear();
			for (auto c : sections[i]) {
				tableau_[i].push_back(card_from_label(c));
			}
		}
		if (sections.size() == tableau_width + 1) {
			auto [st, sc] = kblib::split_first(sections.back(), ':');
			for (auto c : st) {
				append(card_from_label(c));
			}
			if (not sc.empty()) {
				score_ = std::stoi(sc);
			}
		}
		assert(is_possible_tableau());
	}

	friend std::istream& operator>>(std::istream& is, game& g) {
		std::string repr;
		is >> repr;
		g.assign(repr);
		assert(g.is_possible_tableau());
		return is;
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
		os << ':' << g.score_;
		return os;
	}
	game() = default;
	game(std::string_view repr) { assign(repr); }
	template <typename Gen>
	game(Gen&& gen) {
		std::vector<card> deck;
		for (auto i : kblib::range(1, 14)) {
			deck.insert(end(deck), 4, card(i));
		}
		assert(deck.size() == 52);
		std::shuffle(begin(deck), end(deck), gen);
		for (auto s : kblib::range(tableau_.size())) {
			tableau_[s].assign(begin(deck) + tableau_depth * s,
			                   begin(deck) + tableau_depth * (s + 1));
		}
		assert(is_full_tableau());
	}

	std::array<int, ranks + 1> histogram() const {
		std::array<int, ranks + 1> hist{};
		auto inc = [&](card c) {
			if (c >= ace and c <= king) {
				++hist[kblib::etoi(c)];
			} else {
				++hist[0];
			}
		};
		for (auto file : tableau_) {
			for (auto c : file) {
				inc(c);
			}
		}
		for (auto c : stack_) {
			inc(c);
		}
		return hist;
	}
	bool is_full_tableau() const {
		auto hist = histogram();
		return hist[0] == 0 and stack_.empty()
		       and std::all_of(begin(tableau_), end(tableau_),
		                       [](auto& f) { return f.size() == tableau_depth; })
		       and std::all_of(begin(hist) + 1, end(hist),
		                       [](auto c) { return c == 4; });
	}
	bool is_possible_tableau() const {
		auto hist = histogram();
		return hist[0] == 0 and total() <= total_limit
		       and std::all_of(begin(tableau_), end(tableau_),
		                       [](auto& f) { return f.size() <= tableau_depth; })
		       and std::all_of(begin(hist) + 1, end(hist),
		                       [](auto c) { return c <= 4; });
	}
};
game read_deal(std::istream& is) {
	game g;
	is >> g;
	return g;
}

struct solution {
	std::vector<std::uint8_t> moves{};
	int score{};
};
solution solve(game g);

int main(int argc, char** argv) {
	if (argc > 1) {
		for (std::string_view sv : kblib::indirect(&argv[1], &argv[argc])) {
			if (sv == "-") {
				auto g = read_deal(std::cin);
				std::cout << g << '\n' << g.stack_score_ << '\n';
				while (g.append(0u)) {
				}
				std::cout << g << '\n' << g.stack_score_ << '\n';
			} else {
				auto g = game(sv);
				std::cout << g << '\n' << g.stack_score_ << '\n';
				while (g.append(0u)) {
				}
				std::cout << g << '\n' << g.stack_score_ << '\n';
			}
		}
	} else {
		auto seed = std::random_device{}();
		std::cout << "seed: " << seed << '\n';
		auto g = game(kblib::best_lcgs::lcg32(seed));
		std::cout << g << '\n';
	}
}

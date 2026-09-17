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
#include "kblib/iterators.h"
#include "kblib/random.h"
#include "kblib/stringops.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <numeric>
#include <ranges>
#include <vector>

using std::exchange, std::all_of, kblib::range, kblib::etoi;
using std::istream, std::ostream, std::cin, std::cout, std::cerr,
    std::exception, std::invalid_argument;
using std::size_t, std::uint8_t;
using std::string, std::string_view, std::vector, std::array,
    std::unordered_map;
using namespace std::literals;
namespace stdr = std::ranges;
namespace stdv = std::views;

enum card : uint8_t {
	card_null = 0,
	ace = 1,
	ten = 10,
	jack = 11,
	queen = 12,
	king = 13
};
constexpr auto ranks = 13u;
constexpr auto suits = 4u;
constexpr auto deck_size = ranks * suits;

auto value(card c) -> int {
	if (c >= card::jack) {
		return 10;
	} else {
		return static_cast<int>(c);
	}
}
auto card_from_label(char s) -> card {
	static const unordered_map<char, card> map{{'A', ace},   {'0', ten},
	                                           {'X', ten},   {'J', jack},
	                                           {'Q', queen}, {'K', king}};
	if (s >= '0' and s <= '9') {
		return static_cast<card>(s - '0');
	} else {
		return kblib::get_or(map, kblib::toupper(s), card::card_null);
	}
}
auto label_for_card(card c) -> char {
	static const unordered_map<card, char> map{{card_null, '-'}, {ace, 'A'},
	                                           {ten, 'X'},       {jack, 'J'},
	                                           {queen, 'Q'},     {king, 'K'}};
	if (c > ace and c < ten) {
		return static_cast<char>(c + '0');
	} else {
		return kblib::get_or(map, c, '-');
	}
}

auto is_consecutive(stdr::forward_range auto&& r) -> bool {
	if (empty(r)) {
		return true;
	} else {
		return stdr::equal(r, range(*begin(r), *end(r)));
	}
}

struct game {
	static constexpr auto tableau_width = 4u;
	static constexpr auto tableau_depth = 13u;
	static constexpr auto total_limit = 31;
	static constexpr auto score_target = 63;

	array<vector<card>, tableau_width> tableau_{};
	vector<card> stack_{};
	int score_{};
	int stack_score_{};

	auto pop(size_t i) -> card {
		assert(i < tableau_.size() and not tableau_[i].empty());
		return kblib::pop(tableau_[i]);
	}
	auto top(size_t i) const -> card {
		assert(i < tableau_.size());
		if (not tableau_[i].empty()) {
			return tableau_[i].back();
		} else {
			return card::card_null;
		}
	}
	auto top() const -> array<card, tableau_width> {
		return {top(0), top(1), top(2), top(3)};
	}
	auto total() const -> int {
		return std::accumulate(begin(stack_), end(stack_), 0,
		                       [](int a, card c) { return a + value(c); });
	}

	auto can_play(size_t i) const noexcept -> bool {
		assert(i < tableau_.size() and not tableau_[i].empty());
		return can_append(top(i));
	}
	auto can_append(card c) const noexcept -> bool {
		// stack total may not exceed 31
		return total() + value(c) <= total_limit;
	}
	auto can_submit() const noexcept -> bool {
		for (auto c : top()) {
			if (can_append(c)) {
				return false;
			}
		}
		return true;
	}
	auto submit() -> void {
		assert(can_submit());
		score_ += exchange(stack_score_, 0);
		stack_.clear();
	}

	auto play(size_t i) -> void {
		assert(i < tableau_.size() and not tableau_[i].empty());
		assert(can_play(i));
		return append(pop(i));
	}
	auto append(card c) -> void {
		assert(can_append(c));
		const auto t = total() + value(c);
		// first card in stack is a jack = +2 pts
		if (stack_.empty() and c == jack) {
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
		const array same_scores{0, 2, 6, 12};
		stack_score_ += same_scores[same_count];
		stack_.push_back(c);

		// run of 3 to 7 cards, in any order = +3 to +7 pts
		for (auto size : range(std::max(7uz, stack_.size()), 2uz, -1uz)) {
			vector<card> last(end(stack_) - size, end(stack_));
			std::sort(begin(last), end(last));
			if (is_consecutive(last)) {
				stack_score_ += size;
				break;
			}
		}
	}

	auto get_stack_score() const -> int {
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
		for (std::pair<card, int> same{}; auto c : stdv::reverse(stack_)) {
			if (c == same.first) {
				++same.second;
			} else {
				same = {c, 1};
			}
			const array same_scores{0, 2, 6, 12};
			s += same_scores[same.second];
		}
		for (auto size : range(std::max(7uz, stack_.size()), 2uz, -1uz)) {
			vector<card> last(end(stack_) - size, end(stack_));
			std::sort(begin(last), end(last));
			if (is_consecutive(last)) {
				s += size;
				break;
			}
		}
		return s;
	}

	auto assign(string_view repr) -> void {
		auto sections = kblib::split_dsv(repr, ',');
		if (sections.size() < tableau_width
		    or sections.size() > (tableau_width + 1)) {
			throw std::invalid_argument("games must have 4 or 5 sections");
		}
		score_ = 0;
		stack_score_ = 0;
		stack_.clear();
		for (auto i : range(tableau_width)) {
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

	friend auto operator>>(istream& is, game& g) -> istream& {
		string repr;
		is >> repr;
		g.assign(repr);
		assert(g.is_possible_tableau());
		return is;
	}
	friend auto operator<<(ostream& os, const game& g) -> ostream& {
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
	game(string_view repr) { assign(repr); }
	template <typename Gen>
	game(Gen&& gen) {
		vector<card> deck;
		for (auto i : range(1u, ranks + 1u)) {
			deck.insert(end(deck), suits, card(i));
		}
		assert(deck.size() == deck_size);
		std::shuffle(begin(deck), end(deck), gen);
		for (auto s : range(tableau_.size())) {
			tableau_[s].assign(begin(deck) + tableau_depth * s,
			                   begin(deck) + tableau_depth * (s + 1));
		}
		assert(is_full_tableau());
	}

	auto histogram() const -> array<unsigned, ranks + 1> {
		array<unsigned, ranks + 1> hist{};
		auto inc = [&](card c) {
			if (c >= ace and c <= king) {
				++hist[etoi(c)];
			} else {
				++hist[etoi(card_null)];
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
	auto is_full_tableau() const -> bool {
		auto hist = histogram();
		return hist[etoi(card_null)] == 0 and stack_.empty()
		       and std::all_of(begin(tableau_), end(tableau_),
		                       [](auto& f) { return f.size() == tableau_depth; })
		       and std::all_of(begin(hist) + etoi(ace), end(hist),
		                       [](auto c) { return c == suits; });
	}
	auto is_possible_tableau() const -> bool {
		auto hist = histogram();
		return hist[etoi(card_null)] == 0 and total() <= total_limit
		       and std::all_of(begin(tableau_), end(tableau_),
		                       [](auto& f) { return f.size() <= tableau_depth; })
		       and std::all_of(begin(hist) + etoi(ace), end(hist),
		                       [](auto c) { return c <= suits; });
	}
};
auto read_deal(istream& is) -> game {
	game g;
	is >> g;
	return g;
}

struct solution {
	vector<uint8_t> moves{};
	int score{};
	friend auto operator<<(ostream& os, const solution& sol) -> ostream& {
		os << '[';
		for (auto m : sol.moves) {
			os << m;
		}
		return os << ']';
	}
};
auto solve(game g) -> solution;

auto main(int argc, char** argv) -> int {
	if (argc > 1) {
		for (string_view sv : kblib::indirect(&argv[1], &argv[argc])) {
			if (sv == "-") {
				auto g = read_deal(cin);
				cout << g << '\n' << g.stack_score_ << '\n';
				while (g.can_play(0u)) {
					g.play(0u);
				}
				cout << g << '\n' << g.stack_score_ << '\n';
			} else {
				auto g = game(sv);
				cout << g << '\n' << g.stack_score_ << '\n';
				while (g.can_play(0u)) {
					g.play(0u);
				}
				cout << g << '\n' << g.stack_score_ << '\n';
			}
		}
	} else {
		auto seed = std::random_device{}();
		cout << "seed: " << seed << '\n';
		auto g = game(kblib::best_lcgs::lcg32(seed));
		cout << g << '\n';
	}
}

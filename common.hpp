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
#ifndef COMMON_HPP
#define COMMON_HPP

#include "kblib/containers.h"
#include "kblib/direct_map.h"
#include "kblib/iterators.h"
#include "kblib/stats.h"
#include "kblib/stringops.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <csignal>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
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

constexpr inline auto value(card c) -> int {
	if (c >= card::jack) {
		return 10;
	} else {
		return static_cast<int>(c);
	}
}
inline auto card_from_label(char s) -> card {
	static const unordered_map<char, card> map{{'A', ace},   {'0', ten},
	                                           {'X', ten},   {'J', jack},
	                                           {'Q', queen}, {'K', king}};
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

using card_stack = std::basic_string<card>;

struct game {
	static constexpr auto tableau_width = 4u;
	static constexpr auto tableau_depth = 13u;
	static constexpr auto total_limit = 31;
	static constexpr auto score_target = 61;
	using tableau_type = array<card_stack, tableau_width>;

	tableau_type tableau_{};
	card_stack stack_{};
	int score_{};

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
		return accumulate(begin(stack_), end(stack_), 0,
		                  [](int a, card c) { return a + value(c); });
	}

	auto can_play(size_t i) const noexcept -> bool {
		assert(i < tableau_.size());
		return (not tableau_[i].empty()) and can_append(top(i));
	}
	auto can_append(card c) const noexcept -> bool {
		// stack total may not exceed 31
		return c != card_null and total() + value(c) <= total_limit;
	}
	auto can_submit() const noexcept -> bool {
		for (auto c : top()) {
			if (can_append(c)) {
				return false;
			}
		}
		return true;
	}
	auto submit() -> bool {
		if (can_submit()) {
			stack_.clear();
			return true;
		}
		return false;
	}

	// returns true if the stack has been submitted and is now empty
	auto play(size_t i) -> bool {
		assert(i < tableau_.size() and not tableau_[i].empty());
		assert(can_play(i));
		append(pop(i));
		return submit();
	}
	auto append(card c) -> void {
		assert(can_append(c));
		const auto t = total() + value(c);
		// first card in stack is a jack = +2 pts
		if (stack_.empty() and c == jack) {
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
		constexpr static array same_scores{0, 2, 6, 12};
		score_ += same_scores[same_count];

		stack_.push_back(c);
		if (stack_.size() >= 3) {
			int count{};
			std::uint16_t bitset{};
			for (auto n : range(min(kblib::to_signed(stack_.size()), 7z))) {
				std::int32_t c1 = kblib::etoi(stack_.at(stack_.size() - n - 1));
				// if the next card is already present, it cannot be a run
				if (bitset & 1 << c1) {
					break;
				}
				bitset |= 1 << c1;
				auto a = bitset >> std::countr_zero(bitset);
				// generate a mask of bits equal in width to the current length
				auto b = (1 << (n + 1)) - 1;
				if (a == b) {
					count = n + 1;
				}
			}
			if (count >= 3) {
				score_ += count;
			}
		}
	}

	auto score() const -> int { return score_; }
	auto card_sum() const -> int {
		auto op = [](int a, card c) { return a + value(c); };
		auto sum = accumulate(begin(stack_), end(stack_), 0, op);
		for (auto& col : tableau_) {
			sum = accumulate(begin(col), end(col), sum, op);
		}
		return sum;
	}

	auto assign(string_view repr) -> void {
		auto sections = kblib::split_dsv(repr, ',');
		if (sections.size() < tableau_width
		    or sections.size() > (tableau_width + 1)) {
			throw invalid_argument("games must have 4 or 5 sections");
		}
		score_ = 0;
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
		os << ':' << g.score();
		return os;
	}
	game() = default;
	game(const game&) = default;
	game(game&&) = default;
	game& operator=(const game&) = default;
	game& operator=(game&&) = default;
	std::strong_ordering operator<=>(const game&) const = default;

	game(string_view repr) { assign(repr); }
	game(tableau_type tab)
	    : tableau_(tab) {
		assert(is_possible_tableau());
	}
	game(tableau_type tab, array<unsigned, game::tableau_width> position)
	    : tableau_(tab) {
		for (auto i : range(tableau_width)) {
			assert(i <= tableau_depth);
			tableau_[i].resize(position[i]);
		}
		assert(is_possible_tableau());
	}
	template <typename Gen>
	   requires requires { typename Gen::result_type; }
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

	auto card_count() const -> size_t {
		return tableau_[0].size() + tableau_[1].size() + tableau_[2].size()
		       + tableau_[3].size();
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
		       and all_of(begin(tableau_), end(tableau_),
		                  [](auto& f) { return f.size() == tableau_depth; })
		       and all_of(begin(hist) + etoi(ace), end(hist),
		                  [](auto c) { return c == suits; });
	}
	auto is_possible_tableau() const -> bool {
		auto hist = histogram();
		return hist[etoi(card_null)] == 0 and total() <= total_limit
		       and all_of(begin(tableau_), end(tableau_),
		                  [](auto& f) { return f.size() <= tableau_depth; })
		       and all_of(begin(hist) + etoi(ace), end(hist),
		                  [](auto c) { return c <= suits; });
	}
};
inline auto read_deal(istream& is) -> game {
	game g;
	is >> g;
	return g;
}

struct move {
	uint8_t col;
	card c;
};

constexpr auto print_freq = 500'000;
constexpr auto print_scale = 1'000;
constexpr auto print_suff = 'k';

constexpr bool use_direct_map = false;

using key = std::uint16_t;
constexpr auto key_base = 16;
constexpr auto key_max = key_base * key_base * key_base * key_base - 1;
inline auto key_from_game(const game& g) -> key {
	assert(g.stack_.empty());
	key ret{};
	for (auto col : g.tableau_) {
		ret = ret * key_base + col.size();
	}
	return ret;
}
inline auto count_from_key(key k) -> size_t {
	size_t ret{};
	for (auto _ : range(game::tableau_width)) {
		ret += k % key_base;
		k /= key_base;
	}
	return ret;
}
inline auto count_from_key(const game::tableau_type& tab) {
	return tab[0].size() + tab[1].size() + tab[2].size() + tab[3].size();
}

struct cached_solve {
	vector<move> moves;
	int score{};
	friend auto operator<<(ostream& os, const cached_solve& sol) -> ostream& {
		os << "{score=" << sol.score << ", moves=" << sol.moves.size() << "[";
		for (auto m : sol.moves) {
			os << +m.col + 1 << ':' << m.c << ", ";
		}
		return os << "]}";
	}
};
struct solution
    : game
    , cached_solve {
	using cached_solve::score;
	auto g() & -> game& { return *this; }
	auto g() const& -> const game& { return *this; }
	auto g() && -> game&& { return std::move(*this); }
	auto s() & -> cached_solve& { return *this; }
	auto s() const& -> const cached_solve& { return *this; }
	auto s() && -> cached_solve&& { return std::move(*this); }
	[[nodiscard]] auto play(this solution self, size_t i) -> solution {
		self.do_play(i);
		return self;
	}
	[[nodiscard]] auto play(this solution self, move m) -> solution {
		assert(self.top(m.col) == m.c);
		self.do_play(m.col);
		return self;
	}
	[[nodiscard]] auto play(this solution self, span<const move> ms)
	    -> solution {
		for (auto m : ms) {
			assert(self.top(m.col) == m.c);
			self.do_play(m.col);
		}
		return self;
	}

 private:
	auto do_play(size_t i) -> void {
		assert(can_play(i));
		moves.push_back({static_cast<uint8_t>(i), top(i)});
		game::play(i);
		score = game::score();
	}
};

using cache
    = std::conditional_t<use_direct_map, kblib::direct_map<key, cached_solve>,
                         unordered_map<key, cached_solve>>;

struct solve_context {
	cache mem{};
	solution best_solve{};
	size_t total_leaves{};
	size_t last_printed{};
};

#endif // COMMON_HPP

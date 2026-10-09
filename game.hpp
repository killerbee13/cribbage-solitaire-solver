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
#ifndef GAME_HPP
#define GAME_HPP

#include "card.hpp"
#include "utils.hpp"

#include "kblib/containers.h"
#include "kblib/stats.h"
#include "kblib/stringops.h"

#include <algorithm>
#include <array>
#include <csignal>
#include <iostream>
#include <utility>
#include <vector>

constexpr inline auto print_freq = 500'000;
constexpr inline auto print_scale = 1'000;
constexpr inline auto print_suff = 'k';

struct move {
	uint8_t col : 2;
	card c : 4;
	bool is_submit : 1;
};

// apparently there's a key_t in the global namespace in a system header
using key_type = std::uint16_t;
constexpr inline auto key_base = tableau_depth + 1;
using pos_t = array<unsigned, tableau_width>;

constexpr auto key_from_position(pos_t position) -> key_type {
	key_type ret{};
	for (auto col : position) {
		assert(col <= tableau_depth);
		ret = static_cast<key_type>(ret * key_base + col);
	}
	return ret;
}
constexpr auto position_from_key(key_type k) -> pos_t {
	pos_t ret{};
	for (auto& col : ret) {
		col = k % key_base;
		k /= key_base;
	}
	return ret;
}
constexpr inline auto key_max = key_base * key_base * key_base * key_base - 1;
#define FOR_EACH_TAB(fun) {fun(0), fun(1), fun(2), fun(3)}

template <typename Container,
          size_t N = std::tuple_size_v<std::remove_cvref_t<Container>>,
          typename Fun>
auto for_each_el(Container&& c, Fun f) {
	return []<std::size_t... Is>(Container&& c, Fun f,
	                             std::index_sequence<Is...>) {
		return std::array{f(c[Is])...};
	}(std::forward<Container>(c), std::move(f), std::make_index_sequence<N>{});
}
template <size_t N, typename Fun>
auto for_each_index(Fun f) {
	return []<std::size_t... Is>(Fun f, std::index_sequence<Is...>) {
		return std::array{f(kblib::type_constant_for<Is>{})...};
	}(std::move(f), std::make_index_sequence<N>{});
}

struct game {
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
	auto top() const -> array<card, tableau_width> { return FOR_EACH_TAB(top); }
	auto size(size_t i) const noexcept -> unsigned {
		assert(i < tableau_width);
		return static_cast<unsigned>(tableau_[i].size());
	}
	auto card_count() const -> size_t {
		return size(0) + size(1) + size(2) + size(3);
	}
	auto position() const noexcept -> pos_t { return FOR_EACH_TAB(size); }
	auto key() const noexcept -> key_type {
		assert(stack_.empty());
		key_type ret{};
		for (auto col : tableau_) {
			assert(col.size() <= tableau_depth);
			ret = static_cast<key_type>(ret * key_base + col.size());
		}
		return ret;
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
					count = static_cast<int>(n + 1);
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
		auto sum = total();
		for (auto& col : tableau_) {
			sum = accumulate(begin(col), end(col), sum, op);
		}
		return sum;
	}

	auto assign(string_view repr) -> void {
		auto sections = kblib::split_dsv(repr, ",_ ");
		if (sections.size() < tableau_width
		    or sections.size() > (tableau_width + 1)) {
			throw invalid_argument(kblib::concat(
			    "games must have 4 or 5 sections, got ", sections.size()));
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
	game(tableau_type tab, pos_t position)
	    : tableau_(tab) {
		for (auto i : range(tableau_width)) {
			assert(i <= tableau_depth);
			tableau_[i].resize(position[i]);
		}
		assert(is_possible_tableau());
	}
	game(game g, key_type k)
	    : game(std::move(g)) {
		auto position = position_from_key(k);
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

inline auto count_from_key(key_type k) -> size_t {
	size_t ret{};
	for (auto _ : range(tableau_width)) {
		ret += k % key_base;
		k /= key_base;
	}
	return ret;
}
inline auto count_from_key(const game::tableau_type& tab) {
	return tab[0].size() + tab[1].size() + tab[2].size() + tab[3].size();
}

#endif // GAME_HPP

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
#include "dfs.hpp"
#include "common.hpp"

// #include "kblib/sort.h"

#define DEBUG_CACHE_ENTRY(g, s)                                             \
	do {                                                                     \
		auto count = g.card_count();                                          \
		auto size = s.moves.size();                                           \
		cout << __LINE__ << " " #g "=" << static_cast<const game&>(g)         \
		     << ",count=" << count << ", " #s "="                             \
		     << static_cast<const cached_solve>(s) << std::endl;              \
		if (count != size) {                                                  \
			cout << "error " << count << "c!=" << size << "m\n" << std::flush; \
		}                                                                     \
		assert(count == size);                                                \
	} while (false)
#define DEBUG_EMPTY_TAB(g)                                          \
	do {                                                             \
		cout << __LINE__ << " " #g "=" << static_cast<const game&>(g) \
		     << ",count=" << g.card_count() << '\n';                  \
		if (g.card_count() != 0) {                                    \
			cout << "error tableau not empty\n" << std::flush;         \
		}                                                             \
		assert(g.card_count() == 0);                                  \
	} while (false)                                                  \
	}

namespace dfs {

auto assert_cache_valid([[maybe_unused]] const cache& mem) {
#ifndef NDEBUG
	for (auto [tab, sol] : mem) {
		auto count = count_from_key(tab);
		auto size = sol.moves.size();
		assert(count == size);
		// DEBUG_CACHE_ENTRY(g, solve);
	}
#endif
}

auto solve_t::operator()(solve_context& ctx, solution s_current,
                         int score_prefix, bool force) -> solution {
	vector<solution> options;
	auto s_best = s_current;
	bool is_leaf = true;
	if (s_current.card_count() == 0 or stop_requested) {
		// do nothing
	} else if (s_current.stack_.empty() and not force) {
		if (auto it = ctx.mem.find(s_current.key()); it != end(ctx.mem)) {
			auto& sol = it->second;
			// DEBUG_CACHE_ENTRY(s_current.g(), sol);
			s_best = s_current.play(sol.moves);
			// this counts for one leaf
			// DEBUG_EMPTY_TAB(s_best);
		} else {
			// clear the current moves for the cache
			auto g1 = game{s_current.tableau_};
			auto sol = solve(ctx, {g1}, score_prefix + s_current.score_, true);
			is_leaf = false;
			// DEBUG_CACHE_ENTRY(g1, sol);
			ctx.mem.try_emplace(s_current.key(),
			                    cached_solve{sol.moves, sol.score});
			// assert_cache_valid(ctx.mem);

			// combine the move lists, skip the leaf increment because this is
			// pseudo-tail recursion
			s_best = s_current.play(sol.moves);
			// DEBUG_EMPTY_TAB(s_best);
		}
	} else {
		for (auto i : range(uint8_t{tableau_width})) {
			if (s_current.can_play(i)) {
				options.push_back(s_current.play(i));
			}
		}
		//		kblib::insertion_sort(begin(options), end(options),
		//		                      [](const auto& lhs, const auto& rhs) {
		//			                      return lhs.score > rhs.score;
		//		                      });
		stdr::sort(options, std::greater<>{}, &solution::score);
		for (auto s_next : options) {
			if (auto s_tmp = solve(ctx, s_next, score_prefix);
			    s_tmp.score >= s_best.score) {
				is_leaf = false;
				s_best = std::move(s_tmp);
			}
		}
		// DEBUG_EMPTY_TAB(s_best);
	}

	if (is_leaf) {
		++ctx.total_leaves;
		if (s_best.score + score_prefix > ctx.best_solve.score
		    and s_best.moves.size() == 52) {
			ctx.best_solve = s_best;
			cout << "leaf[" << ctx.total_leaves
			     << "] new best solve: " << ctx.best_solve.s() << '\n';
		}
		if (ctx.total_leaves - ctx.last_printed > print_freq) {
			cout << "leaves: " << ctx.total_leaves / print_scale << print_suff
			     << "; top score: " << ctx.best_solve.score
			     << " (calc: " << score_prefix + s_best.score_
			     << "); suffix length: " << s_current.card_count() << "; subscore "
			     << s_best.g().score() << '\n';
			ctx.last_printed = (ctx.total_leaves / print_freq) * print_freq;
		}
	}
	if (info_requested.exchange(0)) {
		cout << "leaves: " << ctx.total_leaves / print_scale << print_suff
		     << "; best: " << ctx.best_solve.s() << '\n';
	}
	return s_best;
}

// this whole thing could use `key`s instead
auto preprocess_scores(solve_context& ctx, const game& g) -> void {
	using position = array<unsigned, tableau_width>;
	vector<position> positions;
	auto inc = [](position& x) {
		bool carry = true;
		for (auto& p : x) {
			// note that this does allow values of 0 to tableau_depth in each digit
			if ((p += exchange(carry, false)) > tableau_depth) {
				p = 0;
				carry = true;
			} else {
				return false;
			}
		}
		// carry out of digit 0 => exhausted all possibilities
		return carry;
	};
	for (position p = {}; not inc(p);) {
		positions.push_back(p);
	}
	stdr::sort(positions, {}, [](const position& p) {
		return pair(accumulate(begin(p), end(p), 0u), p);
	});
	for (auto p : positions) {
		// if (accumulate(begin(p), end(p), 0u) < 6) {
		// 	continue;
		// }
		if (accumulate(begin(p), end(p), 0u) > 16) {
			break;
		}
		auto g1 = game(g.tableau_, p);
		auto sol = solve(ctx, {g1}, 0);
		// DEBUG_CACHE_ENTRY(g1, sol);
		ctx.mem.try_emplace(g1.key(), cached_solve{sol.moves, sol.score});
		// assert_cache_valid(ctx.mem);
		// cout << "pos: (" << p[0] << ',' << p[1] << ',' << p[2] << ',' << p[3]
		//     << "), score: " << score << '\n';
	}
	return;
}

} // namespace dfs

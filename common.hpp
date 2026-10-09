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
#ifndef COMMON_HPP
#define COMMON_HPP

#include "cache.hpp"
#include "game.hpp"
#include <iomanip>

auto print_time_msg() -> std::ostream&;
auto enumerate_stacks(solve_context& ctx, solution s_current)
    -> vector<inplace_vector<move, 13>>;

template <auto solve>
auto process_deal_impl(game g) -> int {
	auto start = std::chrono::steady_clock::now();
	print_time_msg() << g << '\n';
	if (not g.is_full_tableau()) {
		cout << "Warning: Partial game state!\n";
	}
	auto ctx = std::make_unique<solve_context>(g);
	auto s = solve(*ctx, {g});
	auto finish = std::chrono::steady_clock::now();
	cout << "best solution found " << s.s() << ", multiplicity "
	     << ctx->multiplicity << '\n';
	cout << "searched " << ctx->total_leaves << " solutions, cached "
	     << ctx->mem.size() << " states\n";
	cout << "walkthrough:\n";
	auto g1 = g;
	cout << "init: " << g1 << '\n';
	for (auto m : s.moves) {
		g1.play(m.col);
		cout << +m.col + 1 << '(' << m.c << "): " << g1 << '\n';
	}
	cout << "Deal: " << g.deal() << " Took: " << std::setprecision(7)
	     << std::chrono::duration_cast<std::chrono::duration<float>>(finish
	                                                                 - start)
	            .count()
	     << ' ' << s.rep() << '\n';
	return s.score_;
}

#endif // COMMON_HPP

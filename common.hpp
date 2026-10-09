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

auto print_time_msg() -> std::ostream&;

template <auto solve>
auto process_deal_impl(game g) -> int {
	print_time_msg() << g << '\n' << g.score() << '\n';
	auto ctx = std::make_unique<solve_context>(g);
	auto s = solve(*ctx, {g});
	cout << "best solution found " << s.s() << ", multiplicity "
	     << ctx->multiplicity << '\n';
	cout << "searched " << ctx->total_leaves << " solutions\n";
	cout << "walkthrough:\n";
	auto g1 = g;
	cout << "init: " << g1 << '\n';
	for (auto m : s.moves) {
		g1.play(m.col);
		cout << +m.col + 1 << '(' << m.c << "): " << g1 << '\n';
	}
	return s.score_;
}

#endif // COMMON_HPP

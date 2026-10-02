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
#ifdef NDEBUG
#	undef NDEBUG
#endif

#include "bfs.hpp"
#include "common.hpp"
#include "dfs.hpp"

#include "kblib/direct_map.h"
#include "kblib/iterators.h"
#include "kblib/random.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <csignal>
#include <iostream>
#include <ranges>

extern "C" void sigterm_handler(int signal) { stop_requested = signal; }
extern "C" void siginfo_handler(int signal) { info_requested = signal; }

auto main(int argc, char** argv) -> int {
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);
	signal(SIGUSR1, siginfo_handler);
	signal(SIGUSR2, siginfo_handler);
	signal(SIGTSTP, siginfo_handler);

	if (argc > 1) {
		for (string_view sv : kblib::indirect(&argv[1], &argv[argc])) {
			if (sv == "-") {
				auto g = read_deal(cin);
				dfs::process_deal(std::move(g));
			} else {
				auto g = game(sv);
				dfs::process_deal(std::move(g));
			}
		}
	} else {
		auto seed = std::random_device{}();
		cout << "seed: " << seed << '\n';
		auto g = game(kblib::best_lcgs::lcg32(seed));
		dfs::process_deal(std::move(g));
	}
}

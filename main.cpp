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

#include "common.hpp"
#include "dfs.hpp"

#include "kblib/iterators.h"
#include "kblib/random.h"

#include <cassert>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>

extern "C" void sigterm_handler(int signal) { stop_requested = signal; }
extern "C" void siginfo_handler(int signal) { info_requested = signal; }

namespace ch = std::chrono;
using ch::steady_clock;

auto print_time_msg() -> std::ostream& {
	auto now_seconds = ch::time_point_cast<ch::seconds>(ch::system_clock::now());
	std::println("{:%T}: ", now_seconds);
	return std::cout;
}

auto print_time_delta(steady_clock::time_point begin,
                      steady_clock::time_point end) -> void {
	auto diff = ch::duration_cast<ch::duration<float>>(
	    ch::duration_cast<ch::milliseconds>(end - begin));
	auto now_seconds = ch::time_point_cast<ch::seconds>(ch::system_clock::now());
	if (diff > ch::minutes{1}) {
		std::println("{0:%T}: {1} ({1:%T}) elapsed", now_seconds, diff);
	} else {
		std::println("{0:%T}: {1} elapsed", now_seconds, diff);
	}
	return;
}

auto main(int argc, char** argv) -> int {
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);
	signal(SIGUSR1, siginfo_handler);
	signal(SIGUSR2, siginfo_handler);
	signal(SIGTSTP, siginfo_handler);

	auto start = steady_clock::now();
	int deals_looped{};
	if (argc > 1) {
		auto last_start = start;
		for (string_view sv : kblib::indirect(&argv[1], &argv[argc])) {
			if (sv == "-") {
				auto g = read_deal(cin);
				dfs::process_deal(std::move(g));
			} else {
				auto g = game(sv);
				dfs::process_deal(std::move(g));
			}
			auto last_end = steady_clock::now();
			// print_time_delta(last_start, last_end);
			last_start = last_end;
			++deals_looped;
		}
	} else {
		auto seed = std::random_device{}();
		cout << "seed: " << seed << '\n';
		auto g = game(kblib::best_lcgs::lcg32(seed));
		dfs::process_deal(std::move(g));
	}
	if (deals_looped != 1) {
		auto end = steady_clock::now();
		print_time_delta(start, end);
	}
}

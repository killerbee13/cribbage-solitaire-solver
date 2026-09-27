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

#include "kblib/containers.h"
#include "kblib/hash.h"
#include "kblib/iterators.h"
#include "kblib/random.h"
#include "kblib/stringops.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <csignal>
#include <iostream>
#include <map>
#include <numeric>
#include <ranges>
#include <vector>

using std::exchange, std::all_of, std::accumulate, kblib::min, kblib::max,
    kblib::range, kblib::etoi;
using std::istream, std::ostream, std::cin, std::cout, std::cerr,
    std::exception, std::invalid_argument;
using std::size_t, std::uint8_t;
using std::string, std::string_view, std::vector, std::array,
    std::unordered_map, std::map, std::pair, std::span;
using namespace std::literals;
namespace stdr = std::ranges;
namespace stdv = std::views;

inline std::atomic<std::sig_atomic_t> stop_requested;
inline std::atomic<std::sig_atomic_t> info_requested;

extern "C" inline void sigterm_handler(int signal) { stop_requested = signal; }
extern "C" inline void siginfo_handler(int signal) { info_requested = signal; }

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
	if (s > '0' and s <= '9') {
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
ostream& operator<<(ostream& os, card c) { return os << label_for_card(c); }

auto is_consecutive(stdr::forward_range auto&& r) -> bool {
	if (empty(r)) {
		return true;
	} else {
		return stdr::equal(
		    r, range(*begin(r), static_cast<std::decay_t<decltype(*begin(r))>>(
		                            *(end(r) - 1) + 1)));
	}
}

struct game {
	static constexpr auto tableau_width = 4u;
	static constexpr auto tableau_depth = 13u;
	static constexpr auto total_limit = 31;
	static constexpr auto score_target = 61;
	using tableau_type = array<vector<card>, tableau_width>;

	tableau_type tableau_{};
	vector<card> stack_{};
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
		const array same_scores{0, 2, 6, 12};
		score_ += same_scores[same_count];

		stack_.push_back(c);
		if (stack_.size() >= 3) {
			// std::cout << *this << '\n';
			int count{};
			//			int countf{};
			//			float sum{};
			std::uint16_t bitset{};
			for (auto n : range(min(kblib::to_signed(stack_.size()), 7z))) {
				std::int32_t c1 = kblib::etoi(stack_.at(stack_.size() - n - 1));
				//				cout << label_for_card(card(c1));
				if (bitset & 1 << c1) {
					break;
				}
				bitset |= 1 << c1;
				auto a = bitset >> std::countr_zero(bitset);
				auto b = (1 << (n + 1)) - 1;
				if (a == b) {
					count = n + 1;
				}
				//				cout << ((a == b) ? '+' : '-');
				//				sum += std::bit_cast<float>(c1 << 23);
				//				auto af = std::bit_cast<std::int32_t>(sum) << 9;
				//				auto bf = ~0 << (32 - (n + 1)) << 1;
				//				if (af == bf) {
				//					countf = n + 1;
				//				}
				//				cout << ((af == bf) ? '+' : '-');
			}

			//			for (auto c : stack_) {
			//				cout << label_for_card(c);
			//			}
			//			cout << ' ';
			//			cout << count << " == " << countf << '\n';
			//			assert(count == countf);
			if (count >= 3) {
				score_ += count;
			}
			//			cout << ' ';
			//			int run_len{};
			// run of 3 to 7 cards, in any order = +3 to +7 pts
			//			for (auto size : range(min(7z,
			// kblib::to_signed(stack_.size())), 2z, kblib::decrementer{})) {
			//				vector<card> last(end(stack_) - size, end(stack_));
			//				stdr::sort(last);
			//				//				for (auto c : last) {
			//				//					cout << label_for_card(c);
			//				//				}
			//				//				cout << ' ';
			//				if (is_consecutive(last)) {
			// score_ += size;
			//					//					run_len = size;
			//					break;
			//				}
			//			}
			//			cout << std::endl;
			//			if (not (count < 3 or run_len == count)) {
			//				cout << *this << '\n';
			//				cout << run_len << " == " << count << '\n';
			//			}
			//			assert(count < 3 or run_len == count);
			//			score_ += run_len;
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
auto read_deal(istream& is) -> game {
	game g;
	is >> g;
	return g;
}

struct move {
	uint8_t col;
	card c;
};
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
constexpr auto print_freq = 500'000;
constexpr auto print_scale = 1'000;
constexpr auto print_suff = 'k';

// using cache = map<game::tableau_type, cached_solve>;
using cache = unordered_map<game::tableau_type, cached_solve,
                            kblib::FNV_hash<game::tableau_type>>;
struct solve_context {
	cache mem;
	solution best_solve{};
	size_t total_leaves{};
	size_t last_printed{};
};

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
	} while (false)

auto assert_cache_valid(const cache& mem) {
	for (auto [tab, solve] : mem) {
		auto g = game(tab);
		auto count = g.card_count();
		auto size = solve.moves.size();
		assert(count == size);
		//		DEBUG_CACHE_ENTRY(g, solve);
	}
}

auto solve(solve_context& ctx, solution s_current, int score_prefix,
           bool force = false) -> solution {
	vector<solution> options;
	auto s_best = s_current;
	bool is_leaf = true;
	if (s_current.card_count() == 0 or stop_requested) {
		// do nothing
	} else if (s_current.stack_.empty() and not force) {
		if (auto it = ctx.mem.find(s_current.tableau_); it != end(ctx.mem)) {
			//	if (s_current.card_count() > 4) {
			//		cout << "[" << s_current.score << "+" << it->second.score
			//		     << "] skipping search of last " << s_current.card_count()
			//		     << " cards\n";
			//	}
			//	auto& sol = it->second;
			//	DEBUG_CACHE_ENTRY(s_current.g(), sol);
			s_best = s_current.play(it->second.moves);
			// this counts for one leaf
			//	DEBUG_EMPTY_TAB(s_best);
		} else {
			// clear the current moves for the cache
			auto g1 = game{s_current.tableau_};
			auto sol = solve(ctx, {g1}, score_prefix + s_current.score_, true);
			is_leaf = false;
			// DEBUG_CACHE_ENTRY(g1, sol);
			ctx.mem.try_emplace(s_current.tableau_,
			                    cached_solve{sol.moves, sol.score});
			// assert_cache_valid(ctx.mem);
			// combine the move lists, skip the leaf increment because this is
			// pseudo-tail recursion
			s_best = s_current.play(sol.moves);
			// DEBUG_EMPTY_TAB(s_best);
		}
	} else {
		for (auto i : range(uint8_t{game::tableau_width})) {
			if (s_current.can_play(i)) {
				options.push_back(s_current.play(i));
			}
		}
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

auto preprocess_scores(solve_context& ctx, const game& g) -> void {
	using position = array<unsigned, game::tableau_width>;
	vector<position> positions;
	auto inc = [](position& x) {
		bool carry = true;
		for (auto& p : x) {
			// note that this does allow values of 0 to tableau_depth in each digit
			if ((p += exchange(carry, false)) > game::tableau_depth) {
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
		//		continue;
		// }
		if (accumulate(begin(p), end(p), 0u) > 16) {
			break;
		}
		auto g1 = game(g.tableau_, p);
		auto sol = solve(ctx, {g1}, 0);
		// DEBUG_CACHE_ENTRY(g1, sol);
		ctx.mem.try_emplace(g1.tableau_, cached_solve{sol.moves, sol.score});
		// assert_cache_valid(ctx.mem);
		//	cout << "pos: (" << p[0] << ',' << p[1] << ',' << p[2] << ',' << p[3]
		//	     << "), score: " << score << '\n';
	}
	return;
}

void process_deal(game g) {
	cout << g << '\n' << g.score() << '\n';
	solve_context ctx;
	// preprocess_scores(ctx, g);
	// cout << "preprocessed smallest " << ctx.mem.size() << " board states\n";
	auto s = solve(ctx, {g}, 0);
	cout << "best solution found " << s.s() << '\n';
	cout << "searched " << ctx.total_leaves << " solutions\n";
	cout << "walkthrough:\n";
	auto g1 = g;
	cout << "init: " << g1 << '\n';
	for (auto m : s.moves) {
		g1.play(m.col);
		cout << +m.col + 1 << '(' << m.c << "): " << g1 << '\n';
	}
	return;
}

auto main(int argc, char** argv) -> int {
	// signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);
	signal(SIGUSR1, siginfo_handler);
	signal(SIGUSR2, siginfo_handler);
	signal(SIGTSTP, siginfo_handler);

	if (argc > 1) {
		for (string_view sv : kblib::indirect(&argv[1], &argv[argc])) {
			if (sv == "-") {
				auto g = read_deal(cin);
				process_deal(std::move(g));
			} else {
				auto g = game(sv);
				process_deal(std::move(g));
			}
		}
	} else {
		auto seed = std::random_device{}();
		cout << "seed: " << seed << '\n';
		auto g = game(kblib::best_lcgs::lcg32(seed));
		process_deal(std::move(g));
	}
}

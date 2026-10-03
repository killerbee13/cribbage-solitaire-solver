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
#ifndef CACHE_HPP
#define CACHE_HPP

#include "common.hpp"

#include "utils.hpp"

#include "kblib/fakestd.h"
#include <bitset>
#include <ranges>

struct cached_solve {
	inplace_vector<move, 52> moves;
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

	solution(game g)
	    : game(std::move(g))
	    , cached_solve({}, game::score()) {}

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

class cache_map {
 private:
	struct storage_type {
		std::bitset<key_max + 1> slots{};
		std::array<cached_solve, key_max + 1> map{};
		constexpr auto first() const noexcept -> size_t {
			return slots._Find_first();
		}
		constexpr auto next(key_type k) const noexcept -> size_t {
			return slots._Find_next(k);
		}
	};
	kblib::heap_value<storage_type> data_{kblib::in_place_agg};
	size_t size_;

	static constexpr size_t data_size = sizeof(storage_type);

 public:
	using key_type = ::key_type;
	using mapped_type = cached_solve;
	using value_type = pair<key_type, mapped_type>;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using reference = pair<key_type, mapped_type&>;
	using const_reference = pair<key_type, const mapped_type&>;

	friend class const_iterator;
	friend class iterator;
	class const_iterator {
		friend cache_map;
		const storage_type* data_;
		key_type key_;

	 public:
		using value_type = pair<key_type, const mapped_type&>;
		using reference = value_type;
		using pointer = kblib::containing_ptr<value_type>;

		const_iterator() = default;
		const_iterator(const const_iterator&) = default;
		const_iterator(const_iterator&&) = default;
		auto operator=(const const_iterator&) -> const_iterator& = default;
		auto operator=(const_iterator&&) -> const_iterator& = default;

		explicit const_iterator(const storage_type& s, key_type key)
		    : data_(&s)
		    , key_(key) {}

		auto operator*() const noexcept -> value_type {
			return {key_, data_->map[key_]};
		}
		auto operator->() const noexcept -> pointer {
			return {{key_, data_->map[key_]}};
		}
		auto operator++(int) -> const_iterator {
			const_iterator copy(*this);
			key_ = data_->slots._Find_next(key_);
			return copy;
		}
		auto operator++() -> const_iterator& {
			key_ = data_->slots._Find_next(key_);
			return *this;
		}
		auto operator==(const const_iterator& other) const -> bool = default;
	};
	class iterator : const_iterator {
		friend cache_map;

	 public:
		using value_type = pair<key_type, mapped_type&>;
		using reference = value_type;
		using pointer = kblib::containing_ptr<value_type>;

		iterator() = default;
		iterator(const iterator&) = default;
		iterator(iterator&&) = default;
		auto operator=(const iterator&) -> iterator& = default;
		auto operator=(iterator&&) -> iterator& = default;

		explicit iterator(storage_type& s, key_type key)
		    : const_iterator(s, key) {}

		auto operator*() const noexcept -> value_type {
			return {key_, const_cast<mapped_type&>(data_->map[key_])};
		}
		auto operator->() const noexcept -> pointer {
			return {{key_, const_cast<mapped_type&>(data_->map[key_])}};
		}
		auto operator++(int) -> iterator {
			iterator copy(*this);
			key_ = data_->slots._Find_next(key_);
			return copy;
		}
		auto operator++() -> iterator& {
			key_ = data_->slots._Find_next(key_);
			return *this;
		}
		auto operator==(const iterator& other) const -> bool = default;

	 private:
		iterator(const_iterator it)
		    : const_iterator(it) {}
	};

	using pointer = void;
	using const_pointer = void;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 public:
	cache_map() = default;
	cache_map(const cache_map&) = delete;
	cache_map(cache_map&&) = default;
	auto operator=(const cache_map&) -> cache_map& = delete;
	auto operator=(cache_map&&) -> cache_map& = default;

	constexpr auto begin() noexcept -> iterator { return iterator(cbegin()); }
	constexpr auto begin() const noexcept -> const_iterator { return cbegin(); }
	constexpr auto cbegin() const noexcept -> const_iterator {
		return const_iterator(*data_, data_->first());
	}
	constexpr auto end() noexcept -> iterator { return iterator(cend()); }
	constexpr auto end() const noexcept -> const_iterator { return cend(); }
	constexpr auto cend() const noexcept -> const_iterator {
		return const_iterator(*data_, max_size());
	}

	constexpr auto size() const noexcept -> size_type { return size_; }
	constexpr auto max_size() const noexcept -> size_type { return key_max + 1; }

	constexpr auto empty() const noexcept -> bool { return size_ == 0; }

	constexpr auto find(key_type k) noexcept -> iterator {
		if (data_->slots.test(k)) {
			return iterator(*data_, k);
		} else {
			return end();
		}
	}
	constexpr auto find(key_type k) const noexcept -> const_iterator {
		if (data_->slots.test(k)) {
			return const_iterator(*data_, k);
		} else {
			return end();
		}
	}

	template <typename... Args>
	constexpr auto try_emplace(key_type k, Args&&... args)
	    -> pair<iterator, bool> {
		if (data_->slots.test(k)) {
			return {iterator(*data_, k), false};
		} else {
			data_->slots.set(k);
			data_->map[k] = mapped_type(std::forward<Args>(args)...);
			++size_;
			return {iterator(*data_, k), true};
		}
	}
};

constexpr inline bool use_direct_map = true;
using cache = std::conditional_t<use_direct_map, cache_map,
                                 unordered_map<key_type, cached_solve>>;

struct solve_context {
	game original;
	solution best_solve{original};
	cache mem{};
	size_t total_leaves{};
	size_t last_printed{};

	auto game_from_key(key_type k) -> game { return game(original, k); }
};

template <auto solve>
auto process_deal(game g) -> int {
	cout << g << '\n' << g.score() << '\n';
	auto ctx = std::make_unique<solve_context>(g);
	auto s = solve(*ctx, {g});
	cout << "best solution found " << s.s() << '\n';
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

#endif // CACHE_HPP

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

#include "game.hpp"
#include "utils.hpp"

#include "kblib/fakestd.h"
#include <sstream>

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

	auto rep() const -> std::string {
		std::ostringstream ret;
		ret << "Score: " << score << " Solution: ";
		for (auto m : moves) {
			ret << +m.col + 1;
			if (m.is_submit) {
				ret << '_';
			}
		}
		auto str = std::move(ret).str();
		str.pop_back();
		return str;
	}

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
		moves.push_back({static_cast<uint8_t>(i), top(i), game::play(i)});
		cached_solve::score = game::score();
	}
};

class cache_map {
 private:
	struct storage_type {
		using int_type = unsigned long long;
		static constexpr std::size_t slot_count = key_max + 1;
		static constexpr std::size_t bits_per_word = sizeof(int_type) * CHAR_BIT;

		std::array<int_type, (slot_count - 1) / bits_per_word + 1> slotmap;
		std::array<cached_solve, key_max + 1> map{};

		constexpr static auto index_and_bit(int k) noexcept
		    -> std::pair<unsigned, int_type> {
			auto i = kblib::div(k, bits_per_word);
			return {i.quot, int_type{1} << i.rem};
		}
		auto test(key_type k) const noexcept -> bool {
			assert(k >= 0 and std::cmp_less(k, slot_count));
			// can't use structured binding because std::div_t has unspecified
			// member order
			auto [i, b] = index_and_bit(k);
			return slotmap[i] & b;
		}
		auto set(key_type k) noexcept -> void {
			assert(k >= 0 and std::cmp_less(k, slot_count));
			auto [i, b] = index_and_bit(k);
			slotmap[i] |= b;
			return;
		}

		constexpr auto first() const noexcept -> key_type {
			auto it = std::find_if(slotmap.begin(), slotmap.end(),
			                       [](int_type i) { return i != 0; });
			if (it == slotmap.end()) {
				return slot_count;
			} else {
				return (it - slotmap.begin()) * bits_per_word
				       + std::countr_zero(*it);
			}
		}
		constexpr auto next(key_type k) const noexcept -> key_type {
			if (k == slot_count) {
				return slot_count;
			} else {
				auto [i, b] = index_and_bit(k);
				// start the search from the bit after k%bits_per_word
				if (auto w = slotmap[i] & (b - 1)) {
					return k + std::countr_zero(w);
				}
				auto it = std::find_if(slotmap.begin() + i, slotmap.end(),
				                       [](int_type i) { return i != 0; });
				if (it == slotmap.end()) {
					return slot_count;
				} else {
					return (it - slotmap.begin()) * bits_per_word
					       + std::countr_zero(*it);
				}
			}
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
			key_ = data_->next(key_);
			return copy;
		}
		auto operator++() -> const_iterator& {
			key_ = data_->next(key_);
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
			key_ = data_->next(key_);
			return copy;
		}
		auto operator++() -> iterator& {
			key_ = data_->next(key_);
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
		if (data_->test(k)) {
			return iterator(*data_, k);
		} else {
			return end();
		}
	}
	constexpr auto find(key_type k) const noexcept -> const_iterator {
		if (data_->test(k)) {
			return const_iterator(*data_, k);
		} else {
			return end();
		}
	}

	template <typename... Args>
	constexpr auto try_emplace(key_type k, Args&&... args)
	    -> pair<iterator, bool> {
		if (data_->test(k)) {
			return {iterator(*data_, k), false};
		} else {
			data_->set(k);
			assert(data_->test(k));
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
	size_t multiplicity{};

	auto game_from_key(key_type k) -> game { return game(original, k); }
};

#endif // CACHE_HPP

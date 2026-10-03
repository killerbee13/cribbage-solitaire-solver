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
#ifndef INPLACE_VECTOR_HPP
#define INPLACE_VECTOR_HPP

#include "utils.hpp"

#include "kblib/stringops.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

template <typename T, size_t N>
class inplace_vector : private array<T, N> {
 private:
	std::uint8_t size_{};

 public:
	using value_type = array<T, N>::value_type;
	using reference = array<T, N>::reference;
	using const_reference = array<T, N>::const_reference;
	using size_type = array<T, N>::size_type;
	using difference_type = array<T, N>::difference_type;
	using iterator = array<T, N>::iterator;
	using const_iterator = array<T, N>::const_iterator;
	using reverse_iterator = array<T, N>::reverse_iterator;
	using const_reverse_iterator = array<T, N>::const_reverse_iterator;
	using pointer = array<T, N>::pointer;
	using const_pointer = array<T, N>::const_pointer;

	inplace_vector() = default;
	inplace_vector(const inplace_vector&) = default;
	inplace_vector(inplace_vector&&) = default;
	auto operator=(const inplace_vector&) -> inplace_vector& = default;
	auto operator=(inplace_vector&&) -> inplace_vector& = default;
	explicit constexpr inplace_vector(size_type count, T value = {})
	    : array<T, N>{}
	    , size_{static_cast<uint8_t>(count)} {
		assert(count < max_size());
		array<T, N>::fill(value);
		return;
	}
	template <typename InputIt>
	constexpr inplace_vector(InputIt first, InputIt last)
	    : array<T, N>{} {
		while (first != last and size_ != max_size()) {
			(*this)[size_++] = *first++;
		}
		return;
	}

	friend auto operator<=>(const inplace_vector& lhs, const inplace_vector& rhs)
	    -> std::strong_ordering
	    = default;

	constexpr auto assign(size_type count, T value) noexcept -> void {
		assert(count < max_size());
		array<T, N>::fill({});
		std::fill_n(begin(), count, value);
		size_ = static_cast<uint8_t>(count);
		return;
	}
	template <typename InputIt>
	constexpr auto assign(InputIt first, InputIt last) -> void {
		array<T, N>::fill({});
		size_ = 0;
		while (first != last and size_ != max_size()) {
			(*this)[size_++] = *first++;
		}
		return;
	}

	using array<T, N>::operator[];
	constexpr auto at(size_type pos) const -> const_reference {
		if (pos < size_) {
			return (*this)[pos];
		} else {
			throw std::out_of_range{kblib::concat(
			    "inplace_vector: index out of range [0-", size_, "): ", pos)};
		}
	}
	constexpr auto at(size_type pos) -> reference {
		return const_cast<T&>(const_cast<const inplace_vector*>(this)->at(pos));
	}

	using array<T, N>::begin;
	using array<T, N>::cbegin;
	using array<T, N>::data;
	using array<T, N>::front;
	using array<T, N>::rend;
	using array<T, N>::crend;

	constexpr auto back() -> reference { return (*this)[size_ - 1]; }
	constexpr auto back() const -> const_reference { return (*this)[size_ - 1]; }

	auto end() noexcept -> iterator { return begin() + size_; }
	auto end() const noexcept -> const_iterator { return begin() + size_; }
	auto cend() const noexcept -> const_iterator { return cbegin() + size_; }
	auto rbegin() noexcept -> reverse_iterator {
		return std::make_reverse_iterator(end());
	}
	auto rbegin() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(end());
	}
	auto crbegin() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(end());
	}

	using array<T, N>::max_size;
	constexpr auto size() const noexcept -> size_type { return size_; }
	[[nodiscard]] constexpr auto empty() const noexcept -> bool {
		return size_ == 0;
	}

	constexpr auto clear() noexcept -> void {
		array<T, N>::fill({});
		size_ = 0;
		return;
	}

	constexpr auto erase(const_iterator pos) -> iterator;
	constexpr auto erase(const_iterator first, const_iterator last) -> iterator;

	constexpr auto push_back(T value) -> void {
		assert(size_ < max_size());
		(*this)[size_++] = value;
		return;
	}
	constexpr auto pop_back() -> void {
		assert(size_ > 0);
		--size_;
		return;
	}

	constexpr auto resize(size_type count, T value = {}) -> void {
		if (count < size_) {
			size_ = static_cast<uint8_t>(count);
			std::fill(end(), array<T, N>::end(), T{});
		} else {
			while (size_ != count) {
				(*this)[size_++] = value;
			}
		}
		return;
	}

	constexpr auto swap(inplace_vector& other) -> void;
};

#endif // INPLACE_VECTOR_HPP

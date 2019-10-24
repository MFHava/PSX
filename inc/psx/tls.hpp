
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <thread>
#include <functional>
#include "atomic_forward_list.hpp"

namespace psx {
	template<typename Type, typename Allocator = std::allocator<Type>>
	class tls final {
		using node_type = std::pair<std::thread::id, Type>;
		using list_type = atomic_forward_list<node_type>;

		list_type list;
		const std::function<Type()> init;

		template<bool IsConst>
		class iterator_t final {
			using iterator = std::conditional_t<IsConst, typename list_type::const_iterator, typename list_type::iterator>;
			iterator it;

			friend tls;

			iterator_t(iterator it) noexcept : it{it} {}
		public:
			using iterator_t_category = std::forward_iterator_t_tag;
			using value_type        = Type;
			using difference_type   = std::ptrdiff_t;
			using pointer           = std::conditional_t<IsConst, const Type, Type> *;
			using reference         = std::conditional_t<IsConst, const Type, Type> &;

			iterator_t() noexcept =default;

			operator iterator_t<true>() const noexcept { return it; }

			auto operator++() noexcept -> iterator_t & {
				++it;
				return *this;
			}
			auto operator++(int) noexcept -> iterator_t {
				auto tmp{*this};
				++*this;
				return tmp;
			}

			auto operator*() const noexcept -> reference { return it->second; }
			auto operator->() const noexcept -> pointer { return &**this; }

			friend
			auto operator==(const iterator_t & lhs, const iterator_t & rhs) noexcept { return lhs.it == rhs.it; }
			friend
			auto operator!=(const iterator_t & lhs, const iterator_t & rhs) noexcept { return !(lhs == rhs); }
		};
	public:
		using iterator       = iterator_t<false>;
		using const_iterator = iterator_t<true>;

		template<typename T = Type, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
		tls() : init{[] { return Type{}; }} {}

		template<typename T = Type, typename = std::enable_if_t<std::is_copy_constructible_v<T>>>
		tls(const Type & value) : init{[value] { return value; }} {}

		template<typename T = Type, typename = std::enable_if_t<std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>>>
		tls(Type && value) : init{[value{std::move(value)}] { return value; }} {}

		tls(const tls &) =delete;
		tls(tls &&) noexcept =delete;
		auto operator=(const tls &) -> tls & =delete;
		auto operator=(tls &&) noexcept -> tls & =delete;
		~tls() noexcept =default;

		auto local() -> Type & {
			const auto id{std::this_thread::get_id()};
			if(const auto it{std::find_if(std::begin(list), std::end(list), [&](const auto & p) { return p.first == id; })}; it != std::end(list)) return it->second;
			return list.emplace_front(id, init()).second;
		}

		void clear() noexcept { list.clear(); }

		auto begin() const noexcept { return const_iterator{std::begin(list)}; }
		auto begin()       noexcept { return iterator{std::begin(list)}; }
		auto end() const noexcept { return const_iterator{std::end(list)}; }
		auto end()       noexcept { return iterator{std::end(list)}; }
	};

}



//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <thread>
#include <functional>
#include "atomic_forward_list.hpp"

namespace psx {
	//! @brief temporary thread-local storage
	//! @tparam Type type of thread-local storage
	//! @tparam Allocator allocator to use when allocating the thread-local storage
	template<typename Type, typename Allocator = std::allocator<Type>>
	class tls final {
		static_assert(std::is_copy_constructible_v<Type>);

		using node_type = std::pair<std::thread::id, Type>;
		using list_type = atomic_forward_list<node_type, typename std::allocator_traits<Allocator>::template rebind_alloc<node_type>>;

		list_type list;
		const std::function<Type()> init;

		template<bool IsConst>
		class iterator_t final {
			using iterator = std::conditional_t<IsConst, typename list_type::const_iterator, typename list_type::iterator>;
			iterator it;

			friend tls;

			iterator_t(iterator it) noexcept : it{it} {}
		public:
			using iterator_t_category = std::forward_iterator_tag;
			using value_type        = Type;
			using difference_type   = std::ptrdiff_t;
			using pointer           = std::conditional_t<IsConst, const Type, Type> *;
			using reference         = std::conditional_t<IsConst, const Type, Type> &;

			iterator_t() noexcept =default;

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
		template<typename... Args, typename = std::enable_if_t<std::is_constructible_v<Type, Args...>>>
		tls(
			Args &&... args //!< [in] params to initialize first instance of thread-local storage
		) : init{[value{Type{std::forward<Args>(args)...}}] { return value; }} {}

		tls(const tls &) =delete;
		tls(tls &&) noexcept =delete;
		auto operator=(const tls &) -> tls & =delete;
		auto operator=(tls &&) noexcept -> tls & =delete;
		~tls() noexcept =default;

		//! @brief get access to thread-local storage
		//! @throws ??? any exception thrown by allocator for/copy constructor of thread-local storage
		//! @note allocates thread-local storage on first call from thread
		[[nodiscard]]
		auto local() -> Type & {
			const auto id{std::this_thread::get_id()};
			if(const auto it{std::find_if(std::begin(list), std::end(list), [&](const auto & p) { return p.first == id; })}; it != std::end(list)) return it->second;
			return list.emplace_front(id, init()).second;
		}

		//! @brief clears all thread-local storage
		//! @attention never invoke from concurrent context!
		void clear() noexcept { list.clear(); }

		//! @name Iteration
		//! @brief forward iteration support for thread-local storage
		//! @attention don't invoke concurrently with calls to @ref local
		//! @{
		using const_iterator = iterator_t<true>;
		using iterator       = iterator_t<false>;
		auto begin() const noexcept -> const_iterator { return std::begin(list); }
		auto begin()       noexcept ->       iterator { return std::begin(list); }
		auto end() const noexcept -> const_iterator { return std::end(list); }
		auto end()       noexcept ->       iterator { return std::end(list); }
		//! @}
	};
}

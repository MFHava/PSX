
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <atomic>
#include <type_traits>

namespace psx {
	//TODO: documentation
	template<typename Type, typename Allocator = std::allocator<Type>>
	class atomic_forward_list final {
		static_assert(!std::is_const_v<Type>);
		static_assert(std::is_nothrow_destructible_v<Type>);

		struct node final {
			node(const Type & value) : value{value} {}
			node(Type && value) noexcept(std::is_nothrow_move_constructible_v<Type>) : value{std::move(value)} {}
			template<typename... Args, typename = std::enable_if_t<std::is_constructible_v<Type, Args...>>>
			node(Args &&... args) noexcept(std::is_nothrow_constructible_v<Type, Args...>) : value{std::forward<Args>(args)...} {}

			node * next{nullptr};
			Type value;
		};

		using node_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<node>;
		using node_allocator_traits = std::allocator_traits<node_allocator>;

		std::atomic<node *> head{nullptr};
		[[no_unique_address]] node_allocator alloc;

		template<bool IsConst>
		class iterator_t final {
			node * ptr{nullptr};

			friend atomic_forward_list;

			iterator_t(node * ptr) noexcept : ptr{ptr} {}

			operator iterator_t<false>() const noexcept { return ptr; }
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type        = Type;
			using difference_type   = std::ptrdiff_t;
			using pointer           = std::conditional_t<IsConst, const Type, Type> *;
			using reference         = std::conditional_t<IsConst, const Type, Type> &;

			iterator_t() noexcept =default;

			operator iterator_t<true>() const noexcept { return ptr; }

			auto operator++() noexcept -> iterator_t & {
				assert(ptr);
				ptr = ptr->next;
				return *this;
			}
			auto operator++(int) noexcept -> iterator_t {
				auto tmp{*this};
				++*this;
				return tmp;
			}

			auto operator*() const noexcept -> reference {
				assert(ptr);
				return *ptr;
			}
			auto operator->() const noexcept -> pointer { return &**this; }

			friend
			auto operator==(const iterator_t & lhs, const iterator_t & rhs) noexcept { return lhs.ptr == rhs.ptr; }
			friend
			auto operator!=(const iterator_t & lhs, const iterator_t & rhs) noexcept { return !(lhs == rhs); }
		};
	public:
		using value_type      = Type;
		using allocator_type  = Allocator;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference       = Type &;
		using const_reference = const Type &;
		using pointer         = Type *;
		using const_pointer   = const Type *;
		using iterator        = iterator_t<false>;
		using const_iterator  = iterator_t<true>;

		atomic_forward_list() noexcept =default;
		atomic_forward_list(const atomic_forward_list &) =delete;
		atomic_forward_list(atomic_forward_list && other) noexcept { unsafe_swap(other); }
		auto operator=(const atomic_forward_list &) -> atomic_forward_list & =delete;
		auto operator=(atomic_forward_list && other) noexcept -> atomic_forward_list & { unsafe_swap(other); return *this; }
		~atomic_forward_list() noexcept { clear(); }

		auto push_front(const Type & value) -> Type & { return emplace_front(value); }
		auto push_front(Type && value) -> Type & { return emplace_front(std::move(value)); }
		template<typename... Args>
		auto emplace_front(Args &&... args) -> Type & {
			auto ptr{node_allocator_traits::allocate(alloc, 1)};
			try {
				node_allocator_traits::construct(alloc, ptr, std::forward<Args>(args)...);
			} catch(...) {
				node_allocator_traits::deallocate(alloc, ptr, 1);
				throw;
			}
			ptr->next = head.load();
			while(!head.compare_exchange_weak(ptr->next, ptr));
			return ptr->value;
		}

		void clear() {
			for(auto ptr{head.load()}; ptr;) {
				const auto tmp{ptr};
				ptr = ptr->next;
				node_allocator_traits::destroy(alloc, tmp);
				node_allocator_traits::deallocate(alloc, tmp, 1);
			}
			head = nullptr;
		}

		auto empty() const noexcept -> bool { return !head.load(); }

		void swap(atomic_forward_list & other) noexcept {
			const auto ptr{head.load()};
			head = other.head.load();
			other.head = ptr;
			using std::swap;
			swap(alloc, other.alloc);
		}
		friend
		void swap(atomic_forward_list & lhs, atomic_forward_list & rhs) noexcept { lhs.swap(rhs); }

		auto begin() const noexcept { return const_iterator{head.load()}; }
		auto begin()       noexcept { return       iterator{head.load()}; }
		auto end() const noexcept { return const_iterator{}; }
		auto end()       noexcept { return       iterator{}; }
		auto cbegin() const noexcept { return begin(); }
		auto cend() const noexcept { return end(); }
	};
}


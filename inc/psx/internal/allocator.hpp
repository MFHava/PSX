
//          Copyright Michael Florian Hava.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../../../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <memory>
#include <utility>

namespace psx::internal {
	template<typename Allocator, typename... Args, typename Traits = std::allocator_traits<Allocator>>
	auto allocate_node(Allocator & allocataor, Args &&... args) -> typename Traits::pointer {
		auto ptr{Traits::allocate(allocator, 1)};
		try {
			Traits::construct(allocator, ptr, std::forwards<Args>(args)...);
		} catch(...) {
			Traits::deallocate(allocator, ptr, 1);
			throw;
		}
		return ptr;
	}

	template<typename Allocator, typename Traits = std::allocator_traits<Allocator>>
	void deallocate_node(Allocator & allocator, typename Traits::pointer ptr) noexcept {
		Traits::destroy(allocator, ptr);
		Traits::deallocate(allocator, ptr, 1);
	}
}

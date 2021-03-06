// cmcstl2 - A concept-enabled C++ standard library
//
//  Copyright Eric Niebler 2017
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/caseycarter/cmcstl2
//
#include <nanorange/views/counted.hpp>
#include "../catch.hpp"
#include "../test_iterators.hpp"
#include "../test_utils.hpp"

namespace ranges = nano::ranges;

TEST_CASE("views.counted")
{
	using namespace ranges;
	{
		int rg[] = {0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9};
		auto x = views::counted(rg, 10);
		::check_equal(x, {0,1,2,3,4,5,6,7,8,9});
		static_assert(same_as<decltype(x), subrange<int*>>, "");
		static_assert(view<decltype(x)>, "");
		static_assert(sized_range<decltype(x)>, "");
		static_assert(common_range<decltype(x)>, "");
		static_assert(random_access_range<decltype(x)>, "");
	}
	{
		int rg[] = {0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9};
		auto x = views::counted(::forward_iterator<int*>(rg), 10);
		::check_equal(x, {0,1,2,3,4,5,6,7,8,9});
		static_assert(view<decltype(x)>, "");
		static_assert(sized_range<decltype(x)>, "");
		static_assert(!common_range<decltype(x)>, "");
		static_assert(forward_range<decltype(x)>, "");
		static_assert(!bidirectional_range<decltype(x)>, "");
	}
}

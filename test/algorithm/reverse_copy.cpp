// Range v3 library
//
//  Copyright Eric Niebler 2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
//  Copyright 2005 - 2007 Adobe Systems Incorporated
//  Distributed under the MIT License(see accompanying file LICENSE_1_0_0.txt
//  or a copy at http://stlab.adobe.com/licenses.html)

//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <nanorange/algorithm/reverse_copy.hpp>
#include <nanorange/views/subrange.hpp>
#include <cstring>
#include <utility>
#include "../catch.hpp"
#include "../test_iterators.hpp"
#include "../test_utils.hpp"

namespace stl2 = nano;

namespace {

template <class Iter, class OutIter, class Sent = Iter>
void test()
{
	using P = stl2::reverse_copy_result<Iter, OutIter>;
	// iterators
	{
		const int ia[] = {0};
		const unsigned sa = sizeof(ia) / sizeof(ia[0]);
		int ja[sa] = {-1};
		P p0 = stl2::reverse_copy(Iter(ia), Sent(ia), OutIter(ja));
		::check_equal(ja, {-1});
		CHECK(p0.in == Iter(ia));
		CHECK(base(p0.out) == ja);
		P p1 = stl2::reverse_copy(Iter(ia), Sent(ia + sa), OutIter(ja));
		::check_equal(ja, {0});
		CHECK(p1.in == Iter(ia + sa));
		CHECK(base(p1.out) == ja + sa);

		const int ib[] = {0, 1};
		const unsigned sb = sizeof(ib) / sizeof(ib[0]);
		int jb[sb] = {-1};
		P p2 = stl2::reverse_copy(Iter(ib), Sent(ib + sb), OutIter(jb));
		::check_equal(jb, {1, 0});
		CHECK(p2.in == Iter(ib + sb));
		CHECK(base(p2.out) == jb + sb);

		const int ic[] = {0, 1, 2};
		const unsigned sc = sizeof(ic) / sizeof(ic[0]);
		int jc[sc] = {-1};
		P p3 = stl2::reverse_copy(Iter(ic), Sent(ic + sc), OutIter(jc));
		::check_equal(jc, {2, 1, 0});
		CHECK(p3.in == Iter(ic + sc));
		CHECK(base(p3.out) == jc + sc);

		const int id[] = {0, 1, 2, 3};
		const unsigned sd = sizeof(id) / sizeof(id[0]);
		int jd[sd] = {-1};
		P p4 = stl2::reverse_copy(Iter(id), Sent(id + sd), OutIter(jd));
		::check_equal(jd, {3, 2, 1, 0});
		CHECK(p4.in == Iter(id + sd));
		CHECK(base(p4.out) == jd + sd);
	}

	// ranges
	{
		const int ia[] = {0};
		const unsigned sa = sizeof(ia) / sizeof(ia[0]);
		int ja[sa] = {-1};
		P p0 = stl2::reverse_copy(
				::as_lvalue(stl2::subrange(Iter(ia), Sent(ia))),
				OutIter(ja));
		::check_equal(ja, {-1});
		CHECK(p0.in == Iter(ia));
		CHECK(base(p0.out) == ja);
		P p1 = stl2::reverse_copy(
				::as_lvalue(stl2::subrange(Iter(ia), Sent(ia + sa))),
				OutIter(ja));
		::check_equal(ja, {0});
		CHECK(p1.in == Iter(ia + sa));
		CHECK(base(p1.out) == ja + sa);

		const int ib[] = {0, 1};
		const unsigned sb = sizeof(ib) / sizeof(ib[0]);
		int jb[sb] = {-1};
		P p2 = stl2::reverse_copy(
				::as_lvalue(stl2::subrange(Iter(ib), Sent(ib + sb))),
				OutIter(jb));
		::check_equal(jb, {1, 0});
		CHECK(p2.in == Iter(ib + sb));
		CHECK(base(p2.out) == jb + sb);

		const int ic[] = {0, 1, 2};
		const unsigned sc = sizeof(ic) / sizeof(ic[0]);
		int jc[sc] = {-1};
		P p3 = stl2::reverse_copy(
				::as_lvalue(stl2::subrange(Iter(ic), Sent(ic + sc))),
				OutIter(jc));
		::check_equal(jc, {2, 1, 0});
		CHECK(p3.in == Iter(ic + sc));
		CHECK(base(p3.out) == jc + sc);

		const int id[] = {0, 1, 2, 3};
		const unsigned sd = sizeof(id) / sizeof(id[0]);
		int jd[sd] = {-1};
		P p4 = stl2::reverse_copy(
				::as_lvalue(stl2::subrange(Iter(id), Sent(id + sd))),
				OutIter(jd));
		::check_equal(jd, {3, 2, 1, 0});
		CHECK(p4.in == Iter(id + sd));
		CHECK(base(p4.out) == jd + sd);

		// test rvalue ranges
		std::memset(jd, 0, sizeof(jd));
		auto p5 = stl2::reverse_copy(
				stl2::subrange(Iter(id), Sent(id + sd)), OutIter(jd));
		::check_equal(jd, {3, 2, 1, 0});
		CHECK(p5.in == Iter(id + sd));
		CHECK(base(p4.out) == jd + sd);
	}
}

}

TEST_CASE("alg.reverse_copy")
{
	test<bidirectional_iterator<const int*>, output_iterator<int*> >();
	test<bidirectional_iterator<const int*>, forward_iterator<int*> >();
	test<bidirectional_iterator<const int*>, bidirectional_iterator<int*> >();
	test<bidirectional_iterator<const int*>, random_access_iterator<int*> >();
	test<bidirectional_iterator<const int*>, int*>();

	test<random_access_iterator<const int*>, output_iterator<int*> >();
	test<random_access_iterator<const int*>, forward_iterator<int*> >();
	test<random_access_iterator<const int*>, bidirectional_iterator<int*> >();
	test<random_access_iterator<const int*>, random_access_iterator<int*> >();
	test<random_access_iterator<const int*>, int*>();

	test<const int*, output_iterator<int*> >();
	test<const int*, forward_iterator<int*> >();
	test<const int*, bidirectional_iterator<int*> >();
	test<const int*, random_access_iterator<int*> >();
	test<const int*, int*>();

	test<bidirectional_iterator<const int*>, output_iterator<int*>, sentinel<const int *> >();
	test<bidirectional_iterator<const int*>, forward_iterator<int*>, sentinel<const int *> >();
	test<bidirectional_iterator<const int*>, bidirectional_iterator<int*>, sentinel<const int *> >();
	test<bidirectional_iterator<const int*>, random_access_iterator<int*>, sentinel<const int *> >();
	test<bidirectional_iterator<const int*>, int*>();

	test<random_access_iterator<const int*>, output_iterator<int*>, sentinel<const int *> >();
	test<random_access_iterator<const int*>, forward_iterator<int*>, sentinel<const int *> >();
	test<random_access_iterator<const int*>, bidirectional_iterator<int*>, sentinel<const int *> >();
	test<random_access_iterator<const int*>, random_access_iterator<int*>, sentinel<const int *> >();
	test<random_access_iterator<const int*>, int*>();

	test<const int*, output_iterator<int*>, sentinel<const int *> >();
	test<const int*, forward_iterator<int*>, sentinel<const int *> >();
	test<const int*, bidirectional_iterator<int*>, sentinel<const int *> >();
	test<const int*, random_access_iterator<int*>, sentinel<const int *> >();
	test<const int*, int*>();
}

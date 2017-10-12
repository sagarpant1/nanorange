// nanorange.hpp
//
// Copyright (c) 2017 Tristan Brindle (tcbrindle at gmail dot com)
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef NANORANGE_HPP_INCLUDED
#define NANORANGE_HPP_INCLUDED

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <type_traits>

#ifdef NANORANGE_NO_DEPRECATION_WARNINGS
#define NANORANGE_DEPRECATED
#define NANORANGE_DEPRECATED_FOR(x)
#else
#define NANORANGE_DEPRECATED [[deprecated]]
#define NANORANGE_DEPRECATED_FOR(x) [[deprecated(x)]]
#endif

namespace nanorange {

namespace detail {

inline namespace swap_adl {

using std::swap;

template <typename T, typename U>
constexpr auto adl_swap(T& t, U& u) -> decltype(swap(t, u))
{
    return swap(t, u);
}

} // end inline namespace swap_adl

template <typename T, typename U>
using adl_swap_t = decltype(adl_swap(std::declval<T>(), std::declval<U>()));

/* "Detection idiom" machinery from the Library Fundamentals V2 TS */
template <typename...>
using void_t = void;

struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};

template <class Default, class Void, template <class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};

template <template <class...> class Op, class... Args>
using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

template <class Default, template <class...> class Op, class... Args>
using detected_or = detector<Default, void, Op, Args...>;

template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <class Default, template <class...> class Op, class... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template <class Expected, template <class...> class Op, class... Args>
constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

template <class To, template <class...> class Op, class... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

template <class To, template <class...> class Op, class... Args>
constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;

// "Requires clause" testing machinery
template <typename... Args, typename R, typename = decltype(&R::template requires_<Args...>)>
auto test_requires(R&) -> void;

template <typename R, typename... Args>
using test_requires_t = decltype(detail::test_requires<Args...>(std::declval<R&>()));

template <typename R, typename... Args>
constexpr bool requires_ = detail::is_detected_v<test_requires_t, R, Args...>;

} // namespace detail

/* Core language concepts [7.3] */

#if __cplusplus >= 201700
#define CONCEPT constexpr inline
#else
#define CONCEPT constexpr
#endif

template <typename T, typename U>
CONCEPT bool Same = std::is_same<T, U>::value;

#define REQUIRES(...) std::enable_if_t<__VA_ARGS__, int> = 0


namespace detail {

// Helper function for requires clause SFINAE: is valid only if
// the deduced type is an rvalue T
template <typename T, typename Deduced>
auto same_rv(Deduced&&) -> std::enable_if_t<Same<T, Deduced>>;

template <typename T, typename Deduced>
auto same_lv(Deduced&) -> std::enable_if_t<Same<T, Deduced>>;

}

template <typename T, typename U>
CONCEPT bool DerivedFrom =
        std::is_base_of<U, T>::value &&
        std::is_convertible<std::remove_cv_t<T>*, std::remove_cv_t<U>*>::value;

template <typename From, typename To>
CONCEPT bool ConvertibleTo = std::is_convertible<From, To>::value;

namespace detail {

template <typename T, typename Deduced>
auto convertible_to_rv(Deduced&&) -> std::enable_if_t<ConvertibleTo<Deduced, T>>;

}

// TODO: common_reference_t is a nightmare

template <typename I>
CONCEPT bool Integral = std::is_integral<I>::value;

template <typename I>
CONCEPT bool SignedIntegral = Integral<I> && std::is_signed<I>::value;

template <typename I>
CONCEPT bool UnsignedIntegral = Integral<I> && !SignedIntegral<I>;

template <typename T, typename U>
CONCEPT bool Assignable = std::is_assignable<T, U>::value;

template <typename T>
CONCEPT bool Swappable = detail::is_detected_v<detail::adl_swap_t, T&, T&>;

template <typename T, typename U>
CONCEPT bool SwappableWith =
        detail::is_detected_v<detail::adl_swap_t, T&, U&> &&
        detail::is_detected_v<detail::adl_swap_t, U&, T&>;

template <typename T>
CONCEPT bool Destructible = std::is_nothrow_destructible<T>::value;

template <typename T, typename... Args>
CONCEPT bool Constructible = Destructible<T> &&
        std::is_constructible<T, Args...>::value;

template <typename T>
CONCEPT bool DefaultConstructible = Constructible<T>;

template <typename T>
CONCEPT bool MoveConstructible =
        Constructible<T, T> &&
        ConvertibleTo<T, T>;

template <typename T>
CONCEPT bool CopyConstructible =
        MoveConstructible<T> &&
        Constructible<T, T&> && ConvertibleTo<T&, T> &&
        Constructible<T, const T&> && ConvertibleTo<const T&, T> &&
        Constructible<T, const T> && ConvertibleTo<const T, T>;


/* 7.4 Comparison Concepts */

template <typename T>
CONCEPT bool Movable =
        std::is_object<T>::value &&
        MoveConstructible<T> &&
        Assignable<T&, T> &&
        Swappable<T>;


namespace detail {

struct Boolean_ {
    template <typename B>
    auto requires_(const std::remove_reference_t<B>& b1,
                   const std::remove_reference_t<B>& b2,
                   const bool a) -> decltype (
        convertible_to_rv<bool>(b1),
        convertible_to_rv<bool>(!b1),
        same_rv<bool>(b1 && a),
        same_rv<bool>(b1 || a),
        same_rv<bool>(b1 && b2),
        same_rv<bool>(a && b2),
        same_rv<bool>(b1 || b2),
        same_rv<bool>(a || b2),
        convertible_to_rv<bool>(b1 == b2),
        convertible_to_rv<bool>(b1 == a),
        convertible_to_rv<bool>(a == b2),
        convertible_to_rv<bool>(b1 != b2),
        convertible_to_rv<bool>(b1 != a),
        convertible_to_rv<bool>(a != b2)
    );
};

}

template <typename B>
CONCEPT bool Boolean =
        Movable<std::decay_t<B>> &&
        ConvertibleTo<B, bool> &&
        detail::requires_<detail::Boolean_, B>;

namespace detail {

template <typename Deduced>
auto boolean_rv(Deduced&&) -> std::enable_if_t<Boolean<Deduced>>;

}


namespace detail {

struct WeaklyEqualityComparableWith_ {
    template <typename T, typename U>
    auto requires_(const std::remove_reference_t<T>& t,
                   const std::remove_reference_t<U>& u) -> decltype(
        boolean_rv(t == u),
        boolean_rv(t != u),
        boolean_rv(u == t),
        boolean_rv(u != t)
    );
};

}


template <typename T, typename U>
CONCEPT bool WeaklyEqualityComparableWith =
        detail::requires_<detail::WeaklyEqualityComparableWith_, T, U>;

template <typename T>
CONCEPT bool EqualityComparable = WeaklyEqualityComparableWith<T, T>;

template <typename T, typename U>
CONCEPT bool EqualityComparableWith =
        EqualityComparable<T> &&
        EqualityComparable<U> &&
        WeaklyEqualityComparableWith<T, U>;

namespace detail {

struct StrictTotallyOrdered_ {
    template <typename T>
    auto requires_(const std::remove_reference_t<T>& a,
                   const std::remove_reference_t<T>& b) -> decltype(
        boolean_rv(a < b),
        boolean_rv(a > b),
        boolean_rv(a <= b),
        boolean_rv(a >= b)
    );
};

}

template <typename T>
CONCEPT bool StrictTotallyOrdered =
        EqualityComparable<T> &&
        detail::requires_<detail::StrictTotallyOrdered_, T>;

namespace detail {

struct StrictTotallyOrderedWith_ {
    template <typename T, typename U>
    auto requires_(const std::remove_reference_t<T>& t,
                   const std::remove_reference_t<U>& u) -> decltype(
        boolean_rv(t < u),
        boolean_rv(t > u),
        boolean_rv(t <= u),
        boolean_rv(t >= u),
        boolean_rv(u < t),
        boolean_rv(u > t),
        boolean_rv(u <= t),
        boolean_rv(u >= t)
    );
};

}

template <typename T, typename U>
CONCEPT bool StrictTotallyOrderedWith =
        StrictTotallyOrdered<T> &&
        StrictTotallyOrdered<U> &&
        EqualityComparableWith<T, U> &&
        detail::requires_<detail::StrictTotallyOrderedWith_, T, U>;

/* 7.5 Object Concepts */


template <typename T>
CONCEPT bool Copyable =
        CopyConstructible<T> &&
        Movable<T> &&
        Assignable<T&, const T&>;

template <typename T>
CONCEPT bool Semiregular =
        Copyable<T> &&
        DefaultConstructible<T>;

template <typename T>
CONCEPT bool Regular =
        Semiregular<T> &&
        EqualityComparable<T>;

/* 7.6 Callable Concepts */

namespace detail {

template <typename, typename = void>
struct is_callable : std::false_type {};

template <typename F, typename... Args>
struct is_callable<F(Args...), detail::void_t<std::result_of_t<F&&(
        Args&& ...)>>>
        : std::true_type {};

}

template <typename F, typename... Args>
CONCEPT bool Callable = detail::is_callable<F(Args...)>::value;

template <typename F, typename... Args>
CONCEPT bool RegularCallable = Callable<F, Args...>;

namespace detail {

template <typename, typename = void>
struct is_predicate : std::false_type {};

template <typename F, typename... Args>
struct is_predicate<F(Args...), std::enable_if_t<
                Callable<F, Args...> &&
                Boolean<std::result_of_t<F&&(Args&& ...)>>>>
        : std::true_type {};

}

template <typename F, typename... Args>
CONCEPT bool Predicate = detail::is_predicate<F(Args...)>::value;

template <typename R, typename T, typename U>
CONCEPT bool Relation =
        Predicate<R, T, T> &&
                Predicate<R, U, U> &&
                Predicate<R, T, U> &&
                Predicate<R, U, T>;

template <typename R, typename T, typename U>
CONCEPT bool StrictWeakOrder = Relation<R, T, U>;

/* 9.1 Iterators library */

// FIXME: This is not correct
template <typename E>
auto iter_move(E&& e) -> decltype(std::move(*e))
{
    return std::move(*e);
}

template <typename, typename = void>
struct difference_type {};

template <typename T>
struct difference_type<T*>
    : std::enable_if<std::is_object<T>::value, std::ptrdiff_t> {};

template <typename I>
struct difference_type<const I> : difference_type<std::decay_t<I>> {};

namespace detail {

template <typename T>
using member_difference_type_t = typename T::difference_type;

}

template <typename T>
struct difference_type<T, detail::void_t<typename T::difference_type>> {
    using type = typename T::difference_type;
};

template <typename T>
struct difference_type<T, std::enable_if_t<
        !std::is_pointer<T>::value &&
        !detail::is_detected_v<detail::member_difference_type_t, T> &&
        Integral<decltype(std::declval<const T&>() - std::declval<const T&>())>>>
    : std::make_signed<decltype(std::declval<T>() - std::declval<T>())> {};

template <typename T>
using difference_type_t = typename difference_type<T>::type;

template <typename, typename = void>
struct value_type {};

template <typename T>
struct value_type<T*>
    : std::enable_if<std::is_object<T>::value, std::remove_cv_t<T>> { };

template <typename I>
struct value_type<I, std::enable_if_t<std::is_array<I>::value>>
    : value_type<std::decay_t<I>> {};

template <typename I>
struct value_type<const I> : value_type<std::decay_t<I>> {};

namespace detail {

template <typename T>
using member_value_type_t = typename T::value_type;

template <typename T>
using member_element_type_t = typename T::element_type;

}

template <typename T>
struct value_type<T, std::enable_if_t<detail::is_detected_v<detail::member_value_type_t, T>>>
    : std::enable_if<std::is_object<typename T::value_type>::value, typename T::value_type> {};

template <typename T>
struct value_type<T, std::enable_if_t<detail::is_detected_v<detail::member_element_type_t, T>>>
    : std::enable_if<std::is_object<typename T::element_type>::value,
                     std::remove_cv_t<typename T::element_type>> {};

template <typename T>
using value_type_t = typename value_type<T>::type;

// N.B. we use std::iterator_traits here, rather than a nested type
template <typename T>
using iterator_category_t = typename std::iterator_traits<T>::iterator_category;

template <typename T>
using reference_t = decltype(*std::declval<T&>());

template <typename T>
using rvalue_reference_t = decltype(nanorange::iter_move(std::declval<T&>()));

template <typename In>
CONCEPT bool Readable =
        detail::is_detected_v<value_type_t, In> &&
        detail::is_detected_v<reference_t, In> &&
        detail::is_detected_v<rvalue_reference_t, In>;


namespace detail {

struct Writable_ {
    template <typename Out, typename T>
    auto requires_(Out&& o, T&& t) -> decltype(
        *o = std::forward<T>(t),
        const_cast<const reference_t<Out>&&>(*o) = std::forward<T>(t),
        const_cast<const reference_t<Out>&&>(*std::forward<Out>(o)) = std::forward<T>(t)
    );
};

}

template <typename Out, typename T>
CONCEPT bool Writable = detail::requires_<detail::Writable_, Out, T>;

namespace detail {

struct WeaklyIncrementable_ {
    template <typename I>
    auto requires_(I i) -> decltype(
        difference_type_t<I>{},
        std::enable_if_t<SignedIntegral<difference_type_t<I>>, int>{},
        same_lv<I>(++i),
        i++
    );
};

}

template <typename I>
CONCEPT bool WeaklyIncrementable =
        Semiregular<I> &&
        detail::requires_<detail::WeaklyIncrementable_, I>;


namespace detail {

struct Incrementable_ {
    template <typename I>
    auto requires_(I i) -> decltype(
        same_rv<I>(i++)
    );
};

}

template <typename I>
CONCEPT bool Incrementable =
        Regular<I> &&
        WeaklyIncrementable<I> &&
        detail::requires_<detail::Incrementable_, I>;


namespace detail {

template <typename T>
struct legacy_iterator_traits {
    using value_type_t = typename std::iterator_traits<T>::value_type;
    using difference_t = typename std::iterator_traits<T>::difference_type;
    using reference_t = typename std::iterator_traits<T>::reference;
    using pointer = typename std::iterator_traits<T>::pointer;
    using iterator_category = typename std::iterator_traits<T>::iterator_category;
};

template <typename T>
auto not_void(T&&) -> void;

struct Iterator_ {
    template <typename I>
    auto requires_(I i) -> decltype (
        not_void(*i)
    );
};

}

template <typename I>
CONCEPT bool Iterator =
        detail::requires_<detail::Iterator_, I> &&
        WeaklyIncrementable<I> &&
        detail::is_detected_v<detail::legacy_iterator_traits, I>;

template <typename S, typename I>
CONCEPT bool Sentinel =
        Semiregular<S> &&
        Iterator<I> &&
        WeaklyEqualityComparableWith<S, I> &&
        Same<S, I>;

template <typename S, typename I>
constexpr bool disable_sized_sentinel = false;

namespace detail {

struct SizedSentinel_ {
    template <typename S, typename I>
    auto requires_(const I& i, const S& s) -> decltype(
        same_rv<difference_type_t<I>>(s - i),
        same_rv<difference_type_t<I>>(i - s)
    );
};

}

template <typename S, typename I>
CONCEPT bool SizedSentinel =
        Sentinel<S, I> &&
        !disable_sized_sentinel<std::remove_cv_t<S>, std::remove_cv_t<I>> &&
        detail::requires_<detail::SizedSentinel_, S, I>;

template <typename I>
CONCEPT bool InputIterator =
        Iterator<I> &&
        Readable<I> &&
        DerivedFrom<detail::detected_t<iterator_category_t, I>, std::input_iterator_tag>;

namespace detail {

struct OutputIterator_ {
    template <typename I, typename T>
    auto requires_(I i, T&& t) -> decltype(
        *i++ = std::forward<T>(t)
    );
};

}

template <typename I, typename T>
CONCEPT bool OutputIterator =
        Iterator<I> &&
        Writable<I, T> &&
        detail::requires_<detail::OutputIterator_, I, T>;


template <typename I>
CONCEPT bool ForwardIterator =
        InputIterator<I> &&
        ConvertibleTo<detail::detected_t<iterator_category_t, I>, std::forward_iterator_tag> &&
        Incrementable<I> &&
        Sentinel<I, I>;

namespace detail {

struct BidirectionalIterator_{
    template <typename I>
    auto requires_(I i) -> decltype(
        same_lv<I>(--i),
        same_rv<I>(i--)
    );
};


}

template <typename I>
CONCEPT bool BidirectionalIterator =
    ForwardIterator<I> &&
            DerivedFrom<detail::detected_t<iterator_category_t, I>, std::bidirectional_iterator_tag> &&
            detail::requires_<detail::BidirectionalIterator_, I>;

namespace detail {

struct RandomAccessIterator_{
    template <typename I>
    auto requires_(I i, const I j, const difference_type_t<I> n) -> decltype(
        same_lv<I>(i += n),
        same_rv<I>(j + n),
        same_rv<I>(n + j),
        same_lv<I>(i -= n),
        same_rv<I>(j - n),
        j[n],
        std::enable_if_t<Same<decltype(j[n]), reference_t<I>>, int>{}
    );
};

}

template <typename I>
CONCEPT bool RandomAccessIterator =
        BidirectionalIterator<I> &&
        DerivedFrom<detail::detected_t<iterator_category_t, I>, std::random_access_iterator_tag> &&
        StrictTotallyOrdered<I> &&
        SizedSentinel<I, I> &&
        detail::requires_<detail::RandomAccessIterator_, I>;


/* 9.4 Indirect callable concepts */

template <typename F, typename I>
CONCEPT bool IndirectUnaryCallable =
        Readable<I> &&
        CopyConstructible<F> &&
        Callable<F&, value_type_t<I>> &&
        Callable<F&, reference_t<I>>;

template <typename F, typename I>
CONCEPT bool IndirectRegularUnaryCallable =
        Readable<I> &&
        CopyConstructible<F> &&
        RegularCallable<F&, value_type_t<I>> &&
        RegularCallable<F&, reference_t<I>>;

template <typename F, typename I>
CONCEPT bool IndirectUnaryPredicate =
        Readable<I> &&
        CopyConstructible<F> &&
        Predicate<F&, value_type_t<I>&> &&
        Predicate<F&, reference_t<I>>;

template <typename F, typename I1, typename I2 = I1>
CONCEPT bool IndirectRelation =
        Readable<I1> && Readable<I2> &&
        CopyConstructible<F> &&
        Relation<F&, value_type_t<I1>&, value_type_t<I2>&> &&
        Relation<F&, value_type_t<I1>&, reference_t<I2>> &&
        Relation<F&, reference_t<I1>, value_type_t<I2>&> &&
        Relation<F&, reference_t<I1>, reference_t<I2>>;

template <typename F, typename I1, typename I2 = I1>
CONCEPT bool IndirectStrictWeakOrder =
        Readable<I1> && Readable<I2> &&
        CopyConstructible<F> &&
        StrictWeakOrder<F&, value_type_t<I1>&, value_type_t<I2>&> &&
        StrictWeakOrder<F&, value_type_t<I1>&, reference_t<I2>> &&
        StrictWeakOrder<F&, reference_t<I1>, value_type_t<I2>&> &&
        StrictWeakOrder<F&, reference_t<I1>, reference_t<I2>>;

template <typename, typename = void> struct indirect_result_of {};

template <typename F, typename... Is>
struct indirect_result_of<F(Is...), std::enable_if_t<Callable<F, reference_t<Is>...>>>
    : std::result_of<F(reference_t<Is>&&...)> {};

template <typename F>
using indirect_result_of_t = typename indirect_result_of<F>::type;


/* 9.5 Common algorithm requirements */

template <typename In, typename Out>
CONCEPT bool IndirectlyMovable =
        Readable<In> &&
        Writable<Out, rvalue_reference_t<In>>;

template <typename In, typename Out>
CONCEPT bool IndirectlyMovableStorable =
        IndirectlyMovable<In, Out> &&
        Writable<Out, value_type_t<In>> &&
        Movable<value_type_t<In>> &&
        Constructible<value_type_t<In>, rvalue_reference_t<In>> &&
        Assignable<value_type_t<In>&, rvalue_reference_t<In>>;

template <typename In, typename Out>
CONCEPT bool IndirectlyCopyable =
        Readable<In> &&
        Writable<Out, reference_t<In>>;

template <typename In, typename Out>
CONCEPT bool IndirectlyCopyableStorable =
        IndirectlyCopyable<In, Out> &&
        Writable<Out, const value_type_t<In>&> &&
        Copyable<value_type_t<In>> &&
        Constructible<value_type_t<In>, reference_t<In>> &&
        Assignable<value_type_t<In>&, reference_t<In>>;


template <typename I1, typename I2 = I1>
CONCEPT bool IndirectlySwappable =
        Readable<I1> && Readable<I2>; // FIXME: add iter_swap requirements

template <typename I1, typename I2, typename R = std::equal_to<>>
CONCEPT bool IndirectlyComparable =
        IndirectRelation<R, I1, I2>;

template <typename I>
CONCEPT bool Permutable =
        ForwardIterator<I> &&
        IndirectlyMovableStorable<I, I> &&
        IndirectlySwappable<I, I>;

template <typename I1, typename I2, typename Out, typename R = std::less<>>
CONCEPT bool Mergeable =
        InputIterator<I1> &&
        InputIterator<I2> &&
        WeaklyIncrementable<Out> &&
        IndirectlyCopyable<I1, Out> &&
        IndirectlyCopyable<I2, Out> &&
        IndirectStrictWeakOrder<R, I1, I2>;

template <typename I, typename R = std::less<>>
CONCEPT bool Sortable =
        Permutable<I> &&
        IndirectStrictWeakOrder<R, I>;

/* 10.0 Ranges Library */

/* 10.2 DECAY_COPY */

namespace detail {

template <typename T>
auto decay_copy(T&& x)
    noexcept(noexcept(std::decay_t<decltype((x))>(x)))
    -> decltype(std::decay_t<decltype((x))>(x))
{
    return std::decay_t<decltype((x))>(x);
}

}

/* 10.4 Range access */

namespace detail {

template <typename T>
constexpr T static_const_{};

// 10.4.1 begin

namespace begin_ {

template <typename T>
void begin(T&) = delete;

template <typename T>
using member_begin_t = decltype(std::declval<T&>().begin());

template <typename T>
constexpr bool has_member_begin_v =
        Iterator<detected_t<member_begin_t, T>>;

template <typename T>
using nonmember_begin_t = decltype(begin(std::declval<T&>()));

template <typename T>
constexpr bool has_nonmember_begin_v =
        Iterator<detected_t<nonmember_begin_t, T>>;

struct begin_cpo {

    template <typename T, std::size_t N>
    constexpr auto operator()(T (& t)[N]) const noexcept
    -> decltype((t) + 0)
    {
        return (t) + 0;
    }

    template <typename T,
            REQUIRES(has_member_begin_v<T>)>
    constexpr auto operator()(T& t) const
    noexcept(noexcept(decay_copy(t.begin())))
    -> decltype(decay_copy(t.begin()))
    {
        return decay_copy(t.begin());
    }

    template <typename T,
            REQUIRES(has_nonmember_begin_v<T> &&
                             !has_member_begin_v<T>)>
    constexpr auto operator()(T& t) const
    noexcept(noexcept(decay_copy(begin(t))))
    -> decltype(decay_copy(begin(t)))
    {
        return decay_copy(begin(t));
    }

    template <typename T,
            REQUIRES(!std::is_array<T>::value &&
                    (has_member_begin_v<const T> ||
                     has_nonmember_begin_v<const T>))>
    NANORANGE_DEPRECATED_FOR(
            "Calling begin() with an rvalue range is deprecated")
    constexpr decltype(auto) operator()(const T&& t) const
    noexcept(noexcept(std::declval<const begin_cpo&>()(
            static_cast<const T&>(t))))
    {
        return (*this)(static_cast<const T&>(t));
    }
};

} // end namespace begin_
} // end namespace detail

namespace {

constexpr const auto& begin = detail::static_const_<detail::begin_::begin_cpo>;

}



// 10.4.2 end

namespace detail {
namespace end_ {

    template <typename T>
    void end(T&) = delete;

    template <typename T> using begin_t = decltype(nanorange::begin(std::declval<T&>()));

    template <typename T>
    using member_end_t = decltype(std::declval<T&>().end());

    template <typename T>
    constexpr bool has_member_end_v =
            Sentinel<detected_t<member_end_t, T>,
                     detected_t<begin_t, T>>;

    template <typename T>
    using nonmember_end_t = decltype(end(std::declval<T&>()));

    template <typename T>
    constexpr bool has_nonmember_end_v =
            Sentinel<detected_t<nonmember_end_t, T>,
                     detected_t<begin_t, T>>;

struct end_cpo {

    template <typename T, std::size_t N>
    constexpr auto operator()(T (&t)[N]) const noexcept
    -> decltype((t) + N)
    {
        return (t) + N;
    }

    template <typename T,
            REQUIRES(has_member_end_v<T>)>
    constexpr auto operator()(T& t) const
    noexcept(noexcept(decay_copy(t.end())))
    -> decltype(decay_copy(t.end()))
    {
        return decay_copy(t.end());
    }

    template <typename T,
            REQUIRES(has_nonmember_end_v<T> &&
                     !has_member_end_v<T>)>
    constexpr auto operator()(T& t) const
    noexcept(noexcept(decay_copy(end(t))))
        -> decltype(decay_copy(end(t)))
    {
        return decay_copy(end(t));
    }

    template <typename T,
            REQUIRES(!std::is_array<T>::value &&
                    (has_member_end_v<const T>  ||
                     has_nonmember_end_v<const T>))>
    NANORANGE_DEPRECATED_FOR("Calling end() with an rvalue range is deprecated")
    constexpr decltype(auto) operator()(const T&& t) const
    noexcept(noexcept(std::declval<const end_cpo&>()(static_cast<const T&>(t))))
    {
        return (*this)(static_cast<const T&>(t));
    }
};

} // end namespace end_

} // end namespace detail

namespace {

constexpr const auto& end = detail::static_const_<detail::end_::end_cpo>;

}

namespace detail {

namespace cbegin_ {

template <typename T> using begin_t = decltype(nanorange::begin(std::declval<const T&>()));

struct cbegin_cpo {

    template <typename T,
              REQUIRES(is_detected_v<begin_t, T>)>
    constexpr auto operator()(const T& t) const
        noexcept(noexcept(nanorange::begin(t)))
        -> decltype(nanorange::begin(t))
    {
        return nanorange::begin(t);
    }

    template <typename T,
              REQUIRES(is_detected_v<begin_t, T>)>
    NANORANGE_DEPRECATED_FOR("Calling cend() with an rvalue range is deprectated")
    constexpr auto operator()(const T&& t) const
        noexcept(noexcept(nanorange::begin(t)))
        -> decltype(nanorange::begin(t))
    {
        return nanorange::begin(t);
    }
};

} // end namespace cbegin_

} // end namespace detail

namespace {

constexpr auto cbegin = detail::static_const_<detail::cbegin_::cbegin_cpo>;

}

template <typename Rng>
using iterator_t = decltype(nanorange::begin(std::declval<Rng&>()));

template <typename Rng>
using sentinel_t = decltype(nanorange::end(std::declval<Rng&>()));

template <typename Rng>
using range_value_type_t = value_type_t<iterator_t<Rng>>;

template <typename Rng>
using range_difference_type_t = difference_type_t<iterator_t<Rng>>;

// For our purposes, a type is a range if the result of begin() is an
// iterator, and begin() and end() return the same type
template <typename T>
CONCEPT bool Range =
        Iterator<detail::detected_t<iterator_t, T>> &&
        Same<detail::detected_t<iterator_t, T>, detail::detected_t<sentinel_t, T>>;


template <typename T>
CONCEPT bool BoundedRange = Range<T>;

template <typename T>
CONCEPT bool InputRange =
        Range<T> &&
        InputIterator<detail::detected_t<iterator_t, T>>;

template <typename R, typename T>
CONCEPT bool OutputRange =
        Range<R> &&
        OutputIterator<detail::detected_t<iterator_t, R>, T>;

template <typename T>
CONCEPT bool ForwardRange =
        Range<T> &&
        ForwardIterator<detail::detected_t<iterator_t, T>>;

template <typename T>
CONCEPT bool BidirectionalRange =
        Range<T> &&
        BidirectionalIterator<detail::detected_t<iterator_t, T>>;

template <typename T>
CONCEPT bool RandomAccessRange =
        Range<T> &&
        RandomAccessIterator<detail::detected_t<iterator_t, T>>;



template <typename Container>
struct back_insert_iterator
{
    using container_type = Container;
    using difference_type = std::ptrdiff_t;

    /* Backwards compatibility typedefs */
    using value_type = void;
    using reference = void;
    using pointer = void;
    using iterator_category = std::output_iterator_tag;

    constexpr back_insert_iterator() = default;

    explicit back_insert_iterator(Container& x)
        : cont_(std::addressof(x))
    {}

    back_insert_iterator& operator=(const value_type_t<Container>& value)
    {
        cont_->push_back(value);
        return *this;
    }

    back_insert_iterator& operator=(value_type_t<Container>&& value)
    {
        cont_->push_back(std::move(value));
        return *this;
    }

    back_insert_iterator& operator*() { return *this; }
    back_insert_iterator& operator++() { return *this; }
    back_insert_iterator& operator++(int) { return *this; }

private:
    container_type* cont_ = nullptr;
};

template <typename Container>
back_insert_iterator<Container> back_inserter(Container& x)
{
    return back_insert_iterator<Container>(x);
}

template <typename Container>
struct front_insert_iterator
{
    using container_type = Container;
    using difference_type = std::ptrdiff_t;

    /* Backwards compatibility typedefs */
    using value_type = void;
    using reference = void;
    using pointer = void;
    using iterator_category = std::output_iterator_tag;

    constexpr front_insert_iterator() = default;

    explicit front_insert_iterator(Container& x)
            : cont_(std::addressof(x))
    {}

    front_insert_iterator& operator=(const value_type_t<Container>& value)
    {
        cont_->push_front(value);
        return *this;
    }

    front_insert_iterator& operator=(value_type_t<Container>&& value)
    {
        cont_->front_back(std::move(value));
        return *this;
    }

    front_insert_iterator& operator*() { return *this; }
    front_insert_iterator& operator++() { return *this; }
    front_insert_iterator& operator++(int) { return *this; }

private:
    container_type* cont_ = nullptr;
};

template <typename Container>
front_insert_iterator<Container> front_inserter(Container& x)
{
    return front_insert_iterator<Container>(x);
}

template <typename Container>
struct insert_iterator
{
    using container_type = Container;
    using difference_type = std::ptrdiff_t;

    /* Backwards compatibility typedefs */
    using value_type = void;
    using reference = void;
    using pointer = void;
    using iterator_category = std::output_iterator_tag;

    constexpr insert_iterator() = default;

    explicit insert_iterator(Container& x, iterator_t<Container> i)
            : cont_(std::addressof(x)), it_(i)
    {}

    insert_iterator& operator=(const value_type_t<Container>& value)
    {
        cont_->insert(it_, value);
        ++it_;
        return *this;
    }

    insert_iterator& operator=(value_type_t<Container>&& value)
    {
        cont_->push_back(it_, std::move(value));
        ++it_;
        return *this;
    }

    insert_iterator& operator*() { return *this; }
    insert_iterator& operator++() { return *this; }
    insert_iterator& operator++(int) { return *this; }

private:
    container_type* cont_ = nullptr;
    iterator_t<container_type> it_{};
};

template <typename Container>
insert_iterator<Container> inserter(Container& x)
{
    return back_insert_iterator<Container>(x);
}

template <typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
struct ostream_iterator
{
    using char_type = CharT;
    using traits_type = Traits;
    using ostream_type = std::basic_ostream<CharT, Traits>;
    using difference_type = std::ptrdiff_t;

    // Backwards compatibility traits
    using value_type = void;
    using reference = void;
    using pointer = void;
    using iterator_category = std::output_iterator_tag;

    constexpr ostream_iterator() noexcept = default;

    ostream_iterator(ostream_type& os, const CharT* delim = nullptr) noexcept
            : os_(std::addressof(os)), delim_(delim)
    {}

    ostream_iterator& operator=(const T& value)
    {
        *os_ << value;
        if (delim_) {
            *os_ << delim_;
        }
        return *this;
    }

    ostream_iterator& operator*() { return *this; }
    ostream_iterator& operator++() { return *this; }
    ostream_iterator& operator++(int) { return *this; }

private:
    ostream_type* os_ = nullptr;
    const char_type* delim_ = nullptr;
};

template <typename CharT, typename Traits = std::char_traits<CharT>>
struct ostreambuf_iterator {

    using char_type = CharT;
    using traits = Traits;
    using difference_type = std::ptrdiff_t;
    using streambuf_type = std::basic_streambuf<CharT, Traits>;
    using ostream_type = std::basic_ostream<CharT, Traits>;

    // backwards compatibility traits
    using value_type = void;
    using reference = void;
    using pointer = void;
    using iterator_category = std::output_iterator_tag;

    constexpr ostreambuf_iterator() = default;

    ostreambuf_iterator(ostream_type& s) noexcept
        : sbuf_(s.rdbuf())
    {}

    ostreambuf_iterator(streambuf_type* s) noexcept
        : sbuf_(s)
    {}

    ostreambuf_iterator& operator=(char_type c)
    {
        if (!failed()) {
            failed_ = (sbuf_->sputc(c) == traits::eof());
        }
        return *this;
    }

    ostreambuf_iterator& operator*() { return *this; }
    ostreambuf_iterator& operator++() { return *this; }
    ostreambuf_iterator& operator++(int) { return *this; }

    bool failed() const noexcept { return failed_; }

private:
    streambuf_type* sbuf_ = nullptr;
    bool failed_ = false;
};


template <typename T>
struct dangling {

    template <typename U = T,
              REQUIRES(std::is_default_constructible<U>::value)>
    dangling() : value_{} {}

    dangling(T t) : value_(t) {}

    T get_unsafe() { return value_; }

private:
    T value_;
};

template <typename Range>
using safe_iterator_t = std::conditional_t<
        std::is_lvalue_reference<Range>::value,
        iterator_t<Range>, dangling<iterator_t<Range>>>;

// 11.3 Non-modifying sequence operations

// 11.3.1 All of

template <typename Iter, typename UnaryPredicate,
          REQUIRES(InputIterator<Iter> &&
                   IndirectUnaryPredicate<UnaryPredicate, Iter>)>
bool all_of(Iter first, Iter last, UnaryPredicate pred)
{
    return std::all_of(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename UnaryPredicate,
          REQUIRES(InputRange<Range> &&
                   IndirectUnaryPredicate<UnaryPredicate, iterator_t<Range>>)>
bool all_of(Range&& range, UnaryPredicate pred)
{
    return std::all_of(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

// 11.3.2 Any of

template <typename Iter, typename UnaryPredicate,
          REQUIRES(InputIterator<Iter> &&
                   IndirectUnaryPredicate<UnaryPredicate, Iter>)>
bool any_of(Iter first, Iter last, UnaryPredicate pred)
{
    return std::any_of(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename UnaryPredicate,
          REQUIRES(InputRange<Range> &&
                   IndirectUnaryPredicate<UnaryPredicate, iterator_t<Range>>)>
bool any_of(Range&& range, UnaryPredicate pred)
{
    return std::any_of(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

// 11.3.3 None of

template <typename Iter, typename UnaryPredicate,
          REQUIRES(InputIterator<Iter> &&
                   IndirectUnaryPredicate<UnaryPredicate, Iter>)>
bool none_of(Iter first, Iter last, UnaryPredicate pred)
{
    return std::none_of(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename UnaryPredicate,
          REQUIRES(InputRange<Range> &&
                   IndirectUnaryPredicate<UnaryPredicate, iterator_t<Range>>)>
bool none_of(Range&& range, UnaryPredicate pred)
{
    return std::none_of(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

// 11.3.4 For each

template <typename Iter, typename UnaryFunction,
          REQUIRES(InputIterator<Iter> &&
                   IndirectUnaryCallable<UnaryFunction, Iter>)>
UnaryFunction for_each(Iter first, Iter last, UnaryFunction func)
{
    return std::for_each(std::move(first), std::move(last), std::move(func));
}

template <typename Range, typename UnaryFunction,
          REQUIRES(InputRange<Range> &&
                   IndirectUnaryCallable<UnaryFunction, iterator_t<Range>>)>
UnaryFunction for_each(Range&& range, UnaryFunction func)
{
    return std::for_each(nanorange::begin(range), nanorange::end(range),
                         std::move(func));
}

// 11.3.5 Find

template <typename Iter, typename T,
        REQUIRES(InputIterator<Iter> &&
                 IndirectRelation<std::equal_to<>, Iter, const T*>)>
Iter find(Iter first, Iter last, const T& value)
{
    return std::find(std::move(first), std::move(last), value);
}

template <typename Range, typename T,
        REQUIRES(InputRange<Range> &&
                 IndirectRelation<std::equal_to<>, iterator_t<Range>, const T*>)>
safe_iterator_t<Range>
find(Range&& range, const T& value)
{
    return std::find(nanorange::begin(range), nanorange::end(range), value);
}

template <typename Iter, typename UnaryPredicate,
        REQUIRES(InputIterator<Iter> &&
                 IndirectUnaryPredicate<UnaryPredicate, Iter>)>
Iter find_if(Iter first, Iter last, UnaryPredicate pred)
{
    return std::find_if(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename UnaryPredicate,
        REQUIRES(InputRange<Range> &&
                 IndirectUnaryPredicate<UnaryPredicate, iterator_t<Range>>)>
safe_iterator_t<Range>
find_if(Range&& range, UnaryPredicate pred)
{
    return std::find_if(nanorange::begin(range), nanorange::end(range),
                        std::move(pred));
}

template <typename Iter, typename UnaryPredicate,
        REQUIRES(InputIterator<Iter> &&
                 IndirectUnaryPredicate<UnaryPredicate, Iter>)>
Iter find_if_not(Iter first, Iter last, UnaryPredicate&& pred)
{
    return std::find_if_not(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename UnaryPredicate,
        REQUIRES(InputRange<Range> &&
                 IndirectUnaryPredicate<UnaryPredicate, iterator_t<Range>>)>
safe_iterator_t<Range>
find_if_not(Range&& range, UnaryPredicate pred)
{
    return std::find_if_not(nanorange::begin(range), nanorange::end(range),
                            std::move(pred));
}

// 11.3.6 Find end

template <typename Iter1, typename Iter2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(ForwardIterator<Iter1> &&
                 ForwardIterator<Iter2> &&
                 IndirectRelation<BinaryPredicate, Iter1, Iter2>)>
Iter1 find_end(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2, BinaryPredicate pred = {})
{
    return std::find_end(std::move(first1), std::move(last1),
                         std::move(first2), std::move(last2),
                         std::move(pred));
}

template <typename Range1, typename Range2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(ForwardRange<Range1> &&
                 ForwardRange<Range2> &&
                 IndirectRelation<BinaryPredicate, iterator_t<Range1>, iterator_t<Range2>>)>
safe_iterator_t<Range1>
find_end(Range1&& range1, Range2&& range2, BinaryPredicate pred = {})
{
    return std::find_end(nanorange::begin(range1), nanorange::end(range1),
                         nanorange::begin(range2), nanorange::end(range2),
                         std::move(pred));
}

// 11.3.7 Find first of

template <typename Iter1, typename Iter2, typename Pred = std::equal_to<>,
        REQUIRES(InputIterator<Iter1> &&
                 ForwardIterator<Iter2> &&
                 IndirectRelation<Pred, Iter1, Iter2>)>
Iter1 find_first_of(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2, Pred pred = {})
{
    return std::find_first_of(std::move(first1), std::move(last1),
                              std::move(first2), std::move(last2),
                              std::move(pred));
}

template <typename Range1, typename Range2, typename Pred = std::equal_to<>,
        REQUIRES(InputRange<Range1> &&
                 InputRange<Range2> &&
                 IndirectRelation<Pred, iterator_t<Range1>, iterator_t<Range2>>)>
safe_iterator_t<Range1>
find_first_of(Range1&& range1, Range2&& range2, Pred pred = {})
{
    return std::find_first_of(nanorange::begin(range1), nanorange::end(range1),
                              nanorange::begin(range2), nanorange::end(range2),
                              std::move(pred));
}

// 11.3.8 Adjacent find

template <typename Iter, typename Pred = std::equal_to<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectRelation<Pred, Iter, Iter>)>
Iter adjacent_find(Iter first, Iter last, Pred pred = {})
{
    return std::adjacent_find(std::move(first), std::move(last), std::forward<Pred>(pred));
}

template <typename Range, typename Pred = std::equal_to<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectRelation<Pred, iterator_t<Range>, iterator_t<Range>>)>
safe_iterator_t<Range>
adjacent_find(Range&& range, Pred pred = {})
{
    return std::adjacent_find(nanorange::begin(range), nanorange::end(range),
                              std::move(pred));
};

// 11.3.9 Count

template <typename Iter, typename T,
          REQUIRES(InputIterator<Iter> &&
                   IndirectRelation<std::equal_to<>, Iter, const T*>)>
difference_type_t<Iter>
count(Iter first, Iter last, const T& value)
{
    return std::count(std::move(first), std::move(last), value);
}

template <typename Range, typename T,
          REQUIRES(InputRange<Range> &&
                   IndirectRelation<std::equal_to<>, iterator_t<Range>, const T*>)>
range_difference_type_t<Range>
count(Range&& rng, const T& value)
{
    return std::count(nanorange::begin(rng), nanorange::end(rng), value);
}

template <typename Iter, typename UnaryPredicate,
          REQUIRES(InputIterator<Iter> &&
                   IndirectUnaryPredicate<UnaryPredicate, Iter>)>
difference_type_t<Iter>
count_if(Iter first, Iter last, UnaryPredicate pred)
{
    return std::count_if(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename UnaryPredicate,
          REQUIRES(InputRange<Range> &&
                   IndirectUnaryPredicate<UnaryPredicate, iterator_t<Range>>)>
range_difference_type_t<Range>
count_if(Range&& range, UnaryPredicate pred)
{
    return std::count_if(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

// 11.3.10 Mismatch

// N.B. The "three-legged" form of mismatch() is not included in the TS. We
// provide it as an extension for backwards-compatibility, but mark it as
// deprecated
template <typename Iter1, typename Iter2, typename BinaryPredicate = std::equal_to<>,
          REQUIRES(InputIterator<Iter1> &&
                   InputIterator<Iter2> &&
                   IndirectRelation<BinaryPredicate, Iter1, Iter2>)>
NANORANGE_DEPRECATED
std::pair<Iter1, Iter2>
mismatch(Iter1 first1, Iter2 last1, Iter2 first2, BinaryPredicate pred = {})
{
    return std::mismatch(std::move(first1), std::move(last1), std::move(first2), std::move(pred));
}

template <typename Iter1, typename Iter2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(InputIterator<Iter1> &&
                 InputIterator<Iter2> &&
                 IndirectRelation<BinaryPredicate, Iter1, Iter2>)>
std::pair<Iter1, Iter2>
mismatch(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2, BinaryPredicate pred = {})
{
    return std::mismatch(std::move(first1), std::move(last1),
                         std::move(first2), std::move(last2),
                         std::move(pred));
}

template <typename Range1, typename Range2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(InputRange<Range1> &&
                 InputRange<Range2> &&
                 IndirectRelation<BinaryPredicate, iterator_t<Range1>, iterator_t<Range2>>)>
std::pair<safe_iterator_t<Range1>, safe_iterator_t<Range2>>
mismatch(Range1&& range1, Range2&& range2, BinaryPredicate pred = {})
{
    return std::mismatch(nanorange::begin(range1), nanorange::end(range1),
                         nanorange::begin(range2), nanorange::end(range2),
                         std::move(pred));
}

// 11.3.11 Equal

// Again, the three-legged form of equal() is provided as an extension, but
// deprecated
template <typename Iter1, typename Iter2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(InputIterator<Iter1> &&
                 InputIterator<Iter2> &&
                 IndirectlyComparable<Iter1, Iter2, BinaryPredicate>)>
NANORANGE_DEPRECATED
bool equal(Iter1 first1, Iter1 last1, Iter2 first2, BinaryPredicate pred = {})
{
    return std::equal(std::move(first1), std::move(last1),
                      std::move(first2), std::move(pred));
}

template <typename Iter1, typename Iter2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(InputIterator<Iter1> &&
                 InputIterator<Iter2> &&
                 IndirectlyComparable<Iter1, Iter2, BinaryPredicate>)>
bool equal(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2, BinaryPredicate pred = {})
{
    return std::equal(std::move(first1), std::move(last1),
                      std::move(first2), std::move(last2),
                      std::move(pred));
}

template <typename Range1, typename Range2, typename BinaryPredicate = std::equal_to<>,
        REQUIRES(InputRange<Range1> &&
                 InputRange<Range2> &&
                 IndirectlyComparable<iterator_t<Range1>, iterator_t<Range2>, BinaryPredicate>)>
bool equal(Range1&& range1, Range2&& range2, BinaryPredicate pred = {})
{
    return std::equal(nanorange::begin(range1), nanorange::end(range1),
                      nanorange::begin(range2), nanorange::end(range2),
                      std::move(pred));
}

// 11.3.12 Is permutation

// Once again, deprecated extension
template <typename ForwardIt1, typename ForwardIt2, typename Pred = std::equal_to<>,
        REQUIRES(ForwardIterator<ForwardIt1> &&
                 ForwardIterator<ForwardIt2> &&
                 IndirectlyComparable<ForwardIt1, ForwardIt2, Pred>)>
NANORANGE_DEPRECATED
bool is_permutation(ForwardIt1 first1, ForwardIt1 last1, ForwardIt2 first2, Pred pred = {})
{
    return std::is_permutation(std::move(first1), std::move(last1), std::move(first2), std::move(pred));
}

template <typename ForwardIt1, typename ForwardIt2, typename Pred = std::equal_to<>,
        REQUIRES(ForwardIterator<ForwardIt1> &&
                 ForwardIterator<ForwardIt2> &&
                 IndirectlyComparable<ForwardIt1, ForwardIt2, Pred>)>
bool is_permutation(ForwardIt1 first1, ForwardIt1 last1, ForwardIt2 first2, ForwardIt2 last2, Pred pred = {})
{
    return std::is_permutation(std::move(first1), std::move(last1), std::move(first2), std::move(last2), std::move(pred));
}

template <typename ForwardRng1, typename ForwardRng2, typename Pred = std::equal_to<>,
        REQUIRES(ForwardRange<ForwardRng1> &&
                         ForwardRange<ForwardRng2> &&
                         IndirectlyComparable<iterator_t<ForwardRng1>, iterator_t<ForwardRng2>, Pred>)>
bool is_permutation(ForwardRng1&& range1, ForwardRng2&& range2, Pred pred = {})
{
    return std::is_permutation(nanorange::begin(range1), nanorange::end(range2),
                               nanorange::begin(range2), nanorange::end(range2),
                               std::move(pred));
}

// 11.3.13 Search

template <typename Iter1, typename Iter2, typename Pred = std::equal_to<>,
        REQUIRES(ForwardIterator<Iter1> &&
                 ForwardIterator<Iter2> &&
                 IndirectlyComparable<Iter1, Iter2, Pred>)>
Iter1 search(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2, Pred pred = {})
{
    return std::search(std::move(first1), std::move(last1),
                       std::move(first2), std::move(last2), std::move(pred));
}

template <typename Range1, typename Range2, typename Pred = std::equal_to<>,
        REQUIRES(ForwardRange<Range1> &&
                 ForwardRange<Range2> &&
                 IndirectlyComparable<iterator_t<Range1>, iterator_t<Range2>, Pred>)>
safe_iterator_t<Range1>
search(Range1&& range1, Range2&& range2, Pred pred = {})
{
    return std::search(nanorange::begin(range1), nanorange::end(range1),
                       nanorange::begin(range2), nanorange::end(range2),
                       std::move(pred));
}

template <typename Iter, typename T, typename Pred = std::equal_to<>,
          REQUIRES(ForwardIterator<Iter> &&
                   IndirectlyComparable<Iter, const T*, Pred>)>
Iter search_n(Iter first, Iter last, difference_type_t<Iter> count, const T& value, Pred pred = {})
{
    return std::search_n(std::move(first), std::move(last), count, value, std::move(pred));
}

template <typename Range, typename T, typename Pred = std::equal_to<>,
          REQUIRES(ForwardRange<Range> &&
                   IndirectlyComparable<iterator_t<Range>, const T*, Pred>)>
safe_iterator_t<Range>
search_n(Range&& range, difference_type_t<iterator_t<Range>> count, const T& value, Pred pred = {})
{
    return std::search_n(nanorange::begin(range), nanorange::end(range), count, value, std::move(pred));
}


// 11.4 Modifying sequence operations

// 11.4.1 Copy

template <typename Iter1, typename Iter2,
          REQUIRES(InputIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyCopyable<Iter1, Iter2>)>
Iter2 copy(Iter1 first, Iter1 last, Iter2 ofirst)
{
    return std::copy(std::move(first), std::move(last), std::move(ofirst));
}

template <typename Range1, typename Iter2,
          REQUIRES(InputRange<Range1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 copy(Range1&& range, Iter2 ofirst)
{
    return std::copy(nanorange::begin(range), nanorange::end(range), std::move(ofirst));
}

template <typename Iter1, typename Iter2,
        REQUIRES(InputIterator<Iter1> &&
                 WeaklyIncrementable<Iter2> &&
                 IndirectlyCopyable<Iter1, Iter2>)>
Iter2 copy_n(Iter1 first, difference_type_t<Iter1> count, Iter2 ofirst)
{
    return std::copy_n(std::move(first), count, std::move(ofirst));
}

template <typename Iter1, typename Iter2, typename Pred,
          REQUIRES(InputIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectUnaryPredicate<Pred, Iter1> &&
                   IndirectlyCopyable<Iter1, Iter2>)>
Iter2 copy_if(Iter1 first, Iter1 last, Iter2 ofirst, Pred pred)
{
    return std::copy_if(std::move(first), std::move(last), std::move(ofirst), std::move(pred));
}

template <typename Range1, typename Iter2, typename Pred,
          REQUIRES(InputRange<Range1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectUnaryPredicate<Pred, iterator_t<Range1>> &&
                   IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 copy_if(Range1&& range, Iter2 ofirst, Pred&& pred)
{
    return std::copy_if(nanorange::begin(range), nanorange::end(range), std::move(ofirst),
                        std::move(pred));
}

template <typename Iter1, typename Iter2,
          REQUIRES(BidirectionalIterator<Iter1> &&
                   BidirectionalIterator<Iter2> &&
                   IndirectlyCopyable<Iter1, Iter2>)>
Iter2 copy_backward(Iter1 first, Iter1 last, Iter2 olast)
{
    return std::copy_backward(std::move(first), std::move(last), std::move(olast));
}

template <typename Range1, typename Iter2,
          REQUIRES(BidirectionalRange<Range1> &&
                   BidirectionalIterator<Iter2> &&
                   IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 copy_backward(Range1&& range, Iter2 olast)
{
    return std::copy_backward(nanorange::begin(range), nanorange::end(range), std::move(olast));
}

// 11.4.2 Move

template <typename Iter1, typename Iter2,
          REQUIRES(InputIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyMovable<Iter1, Iter2>)>
Iter2 move(Iter1 first, Iter1 last, Iter2 ofirst)
{
    return std::move(std::move(first), std::move(last), std::move(ofirst));
}

template <typename Range1, typename Iter2,
           REQUIRES(InputRange<Range1> &&
                    WeaklyIncrementable<Iter2> &&
                    IndirectlyMovable<iterator_t<Range1>, Iter2>)>
Iter2 move(Range1&& range, Iter2 ofirst)
{
    return std::move(nanorange::begin(range), nanorange::end(range), std::move(ofirst));
}

template <typename Iter1, typename Iter2,
          REQUIRES(BidirectionalIterator<Iter1> &&
                   BidirectionalIterator<Iter2> &&
                   IndirectlyMovable<Iter1, Iter2>)>
Iter2 move_backward(Iter1 first, Iter1 last, Iter2 olast)
{
    return std::move_backward(std::move(first), std::move(last), std::move(olast));
}

template <typename Range1, typename Iter2,
          REQUIRES(BidirectionalRange<Range1> &&
                   BidirectionalIterator<Iter2> &&
                   IndirectlyMovable<iterator_t<Range1>, Iter2>)>
Iter2 move_backward(Range1&& range, Iter2 olast)
{
    return std::move_backward(nanorange::begin(range), nanorange::end(range), std::move(olast));
}

// 11.4.3 Swap

template <typename Iter1, typename Iter2,
        REQUIRES(ForwardIterator<Iter1> &&
                 ForwardIterator<Iter2> &&
                 IndirectlySwappable<Iter1, Iter2>)>
NANORANGE_DEPRECATED
Iter2 swap_ranges(Iter1 first1, Iter1 last1, Iter2 first2)
{
    return std::swap_ranges(std::move(first1), std::move(last1), std::move(first2));
}

template <typename Iter1, typename Iter2,
        REQUIRES(ForwardIterator<Iter1> &&
                 ForwardIterator<Iter2> &&
                 IndirectlySwappable<Iter1, Iter2>)>
std::pair<Iter1, Iter2>
swap_ranges(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2)
{
    while (first1 != last1 && first2 != last2) {
        std::iter_swap(first1, first2);
        ++first1; ++first2;
    }
    return std::make_pair(std::move(first1), std::move(first2));
}

template <typename Range1, typename Range2,
        REQUIRES(ForwardRange<Range1> &&
                 ForwardRange<Range2> &&
                 IndirectlySwappable<iterator_t<Range1>, iterator_t<Range2>>)>
std::pair<safe_iterator_t<Range1>, safe_iterator_t<Range2>>
swap_ranges(Range1&& range1, Range2&& range2)
{
    return nanorange::swap_ranges(nanorange::begin(range1), nanorange::end(range1),
                                  nanorange::begin(range2), nanorange::end(range2));
}

// 11.4.4 Transform

template <typename Iter1, typename Iter2, typename UnaryOp,
        REQUIRES(InputIterator<Iter1> &&
                 WeaklyIncrementable<Iter2> &&
                 CopyConstructible<UnaryOp> &&
                 Writable<Iter2, indirect_result_of_t<UnaryOp&(Iter1)>>)>
Iter2 transform(Iter1 first, Iter1 last, Iter2 ofirst, UnaryOp op)
{
    return std::transform(std::move(first), std::move(last), std::move(ofirst), std::move(op));
}

template <typename Range1, typename Iter2, typename UnaryOp,
        REQUIRES(InputRange<Range1> &&
                 WeaklyIncrementable<Iter2> &&
                 CopyConstructible<UnaryOp> &&
                 Writable<Iter2, indirect_result_of_t<UnaryOp&(iterator_t<Range1>)>>)>
Iter2 transform(Range1&& range, Iter2 ofirst, UnaryOp op)
{
    return std::transform(nanorange::begin(range), nanorange::end(range),
                          std::move(ofirst), std::move(op));
}

template <typename Iter1, typename Iter2, typename Iter3, typename BinOp,
        REQUIRES(InputIterator<Iter1> &&
                 InputIterator<Iter2> &&
                 WeaklyIncrementable<Iter3> &&
                 CopyConstructible<BinOp> &&
                 Writable<Iter3, indirect_result_of_t<BinOp&(Iter1, Iter2)>>)>
NANORANGE_DEPRECATED
Iter3 transform(Iter1 first1, Iter1 last1, Iter2 first2, Iter3 ofirst, BinOp op)
{
    return std::transform(std::move(first1), std::move(last1),
                          std::move(first2), std::move(ofirst), std::move(op));
}

// Hmmm, why doesn't the standard library provide a 6-parameter overload?
template <typename Iter1, typename Iter2, typename Iter3, typename BinOp,
        REQUIRES(InputIterator<Iter1> &&
                 InputIterator<Iter2> &&
                 WeaklyIncrementable<Iter3> &&
                 CopyConstructible<BinOp> &&
                 Writable<Iter3, indirect_result_of_t<BinOp&(Iter1, Iter2)>>)>
Iter3 transform(Iter1 first1, Iter1 last1, Iter2 first2, Iter2 last2, Iter3 ofirst, BinOp op)
{
    while (first1 != last1 && first2 != last2) {
        *ofirst = op(*first1, *first2);
        ++ofirst; ++first1; ++first2;
    }
    return std::move(ofirst);
}

template <typename Range1, typename Range2, typename Iter3, typename BinOp,
        REQUIRES(InputRange<Range1> &&
                 InputRange<Range2> &&
                 WeaklyIncrementable<Iter3> &&
                 CopyConstructible<BinOp> &&
                 Writable<Iter3, indirect_result_of_t<BinOp&(iterator_t<Range1>, iterator_t<Range2>)>>)>
Iter3 transform(Range1&& range1, Range2&& range2, Iter3 ofirst, BinOp op)
{
    return nanorange::transform(nanorange::begin(range1), nanorange::end(range1),
                                nanorange::begin(range2), nanorange::end(range2),
                                std::move(ofirst), std::move(op));
}

// 11.4.5 Replace

template <typename Iter, typename T,
        REQUIRES(InputIterator<Iter> &&
                 Writable<Iter, const T&> &&
                 IndirectRelation<std::equal_to<>, Iter, const T*>)>
void replace(Iter first, Iter last, const T& old_value, const T& new_value)
{
    std::replace(std::move(first), std::move(last), old_value, new_value);
}

template <typename Range, typename T,
        REQUIRES(InputRange<Range> &&
                 Writable<iterator_t<Range>, const T&> &&
                 IndirectRelation<std::equal_to<>, iterator_t<Range>, const T*>)>
void replace(Range&& range, const T& old_value, const T& new_value)
{
    std::replace(nanorange::begin(range), nanorange::end(range),
                 old_value, new_value);
}

template <typename Iter, typename Pred, typename T,
        REQUIRES(InputIterator<Iter> &&
                 Writable<Iter, const T&> &&
                 IndirectUnaryPredicate<Pred, Iter>)>
void replace_if(Iter first, Iter last, Pred pred, const T& new_value)
{
    std::replace_if(std::move(first), std::move(last), std::move(pred), new_value);
}

template <typename Range, typename Pred, typename T,
        REQUIRES(ForwardRange<Range> &&
                 Writable<iterator_t<Range>, const T&> &&
                 IndirectUnaryPredicate<Pred, iterator_t<Range>>)>
void replace_if(Range&& range, Pred pred, const T& new_value)
{
    std::replace_if(nanorange::begin(range), nanorange::end(range),
                    std::move(pred), new_value);
}

template <typename Iter1, typename Iter2, typename T,
        REQUIRES(InputIterator<Iter1> &&
                 OutputIterator<Iter2, const T&> &&
                 IndirectlyCopyable<Iter1, Iter2> &&
                 IndirectRelation<std::equal_to<>, Iter1, const T*>)>
Iter2 replace_copy(Iter1 first, Iter1 last, Iter2 ofirst, const T& old_value, const T& new_value)
{
    return std::replace_copy(std::move(first), std::move(last), std::move(ofirst),
                             old_value, new_value);
}

template <typename Range1, typename Iter2, typename T,
        REQUIRES(InputRange<Range1> &&
                 OutputIterator<Iter2, const T&> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter2> &&
                 IndirectRelation<std::equal_to<>, iterator_t<Range1>, const T*>)>
Iter2 replace_copy(Range1&& range, Iter2 ofirst, const T& old_value, const T& new_value)
{
    return std::replace_copy(nanorange::begin(range), nanorange::end(range),
                             std::move(ofirst), old_value, new_value);
}

template <typename Iter1, typename Iter2, typename Pred, typename T,
        REQUIRES(InputIterator<Iter1> &&
                 OutputIterator<Iter2, const T&> &&
                 IndirectUnaryPredicate<Pred, Iter1> &&
                 IndirectlyCopyable<Iter1, Iter2>)>
Iter2 replace_copy_if(Iter1 first, Iter1 last, Iter2 ofirst, Pred pred, const T& new_value)
{
    return std::replace_copy_if(std::move(first), std::move(last), std::move(ofirst),
                                std::move(pred), new_value);
}

template <typename Range1, typename Iter2, typename Pred, typename T,
        REQUIRES(InputRange<Range1> &&
                 OutputIterator<Iter2, const T&> &&
                 IndirectUnaryPredicate<Pred, iterator_t<Range1>> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 replace_copy_if(Range1&& range, Iter2 ofirst, Pred pred, const T& new_value)
{
    return std::replace_copy_if(nanorange::begin(range), nanorange::end(range),
                                std::move(ofirst), std::move(pred), new_value);
}

// 11.4.6 Fill

template <typename Iter, typename T,
          REQUIRES(OutputIterator<Iter, const T&>)>
void fill(Iter first, Iter last, const T& value)
{
    std::fill(std::move(first), std::move(last), value);
}

template <typename Range, typename T,
          REQUIRES(OutputRange<Range, const T&>)>
void fill(Range&& range, const T& value)
{
    std::fill(nanorange::begin(range), nanorange::end(range), value);
}

template <typename Iter, typename T,
          REQUIRES(OutputIterator<Iter, const T&>)>
Iter fill_n(Iter first, difference_type_t<Iter> count, const T& value)
{
    return std::fill_n(std::move(first), count, value);
}

// 11.4.7 Generate

template <typename Iter, typename Generator,
          REQUIRES(Iterator<Iter> &&
                   CopyConstructible<Generator> &&
                   Callable<Generator&> &&
                   Writable<Iter, std::result_of_t<Generator&()>>)>
void generate(Iter first, Iter last, Generator gen)
{
    std::generate(std::move(first), std::move(last), std::move(gen));
}

template <typename Range, typename Generator,
          REQUIRES(OutputRange<Range, std::result_of_t<Generator&()>> &&
                   CopyConstructible<Generator> &&
                   Callable<Generator&>)>
void generate(Range&& range, Generator gen)
{
    std::generate(nanorange::begin(range), nanorange::end(range), std::move(gen));
}

template <typename Iter, typename Generator,
          REQUIRES(Iterator<Iter> &&
                   CopyConstructible<Generator> &&
                   Callable<Generator&> &&
                   Writable<Iter, std::result_of_t<Generator&()>>)>
Iter generate_n(Iter first, difference_type_t<Iter> count, Generator generator)
{
    return std::generate_n(std::move(first), count, std::move(generator));
}

// 11.4.8 Remove

template <typename Iter, typename T,
          REQUIRES(ForwardIterator<Iter> &&
                   Permutable<Iter> &&
                   IndirectRelation<std::equal_to<>, Iter, const T*>)>
Iter remove(Iter first, Iter last, const T& value)
{
    return std::remove(std::move(first), std::move(last), value);
}

template <typename Range, typename T,
          REQUIRES(ForwardRange<Range> &&
                   Permutable<iterator_t<Range>> &&
                   IndirectRelation<std::equal_to<>, iterator_t<Range>, const T*>)>
safe_iterator_t<Range> remove(Range&& range, const T& value)
{
    return std::remove(nanorange::begin(range), nanorange::end(range), value);
}

template <typename Iter, typename Pred,
          REQUIRES(ForwardIterator<Iter> &&
                   IndirectUnaryPredicate<Pred, Iter> &&
                   Permutable<Iter>)>
Iter remove_if(Iter first, Iter last, Pred pred)
{
    return std::remove_if(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename Pred,
          REQUIRES(ForwardRange<Range> &&
                   IndirectUnaryPredicate<Pred, iterator_t<Range>> &&
                   Permutable<iterator_t<Range>>)>
safe_iterator_t<Range> remove_if(Range&& range, Pred pred)
{
    return std::remove_if(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

template <typename Iter1, typename Iter2, typename T,
          REQUIRES(InputIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyCopyable<Iter1, Iter2> &&
                   IndirectRelation<std::equal_to<>, Iter1, const T*>)>
Iter2 remove_copy(Iter1 first, Iter1 last, Iter2 ofirst, const T& value)
{
    return std::remove_copy(std::move(first), std::move(last),
                            std::move(ofirst), value);
}

template <typename Range1, typename Iter2, typename T,
        REQUIRES(InputRange<Range1> &&
                 WeaklyIncrementable<Iter2> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter2> &&
                 IndirectRelation<std::equal_to<>, iterator_t<Range1>, const T*>)>
Iter2 remove_copy(Range1&& range, Iter2 ofirst, const T& value)
{
    return std::remove_copy(nanorange::begin(range), nanorange::end(range),
                            std::move(ofirst), value);
}

template <typename Iter1, typename Iter2, typename Pred,
          REQUIRES(InputIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectUnaryPredicate<Pred, Iter1> &&
                   IndirectlyCopyable<Iter1, Iter2>)>
Iter2 remove_copy_if(Iter1 first, Iter1 last, Iter2 ofirst, Pred pred)
{
    return std::remove_copy_if(std::move(first), std::move(last),
                               std::move(ofirst), std::move(pred));
}

template <typename Range1, typename Iter2, typename Pred,
          REQUIRES(InputRange<Range1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectUnaryPredicate<Pred, iterator_t<Range1>> &&
                   IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 remove_copy_if(Range1&& range, Iter2 ofirst, Pred pred)
{
    return std::remove_copy_if(nanorange::begin(range), nanorange::end(range),
                               std::move(ofirst), std::move(pred));
}

// 11.4.9 Unique

template <typename Iter, typename Pred = std::equal_to<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectRelation<Pred, Iter> &&
                 Permutable<Iter>)>
Iter unique(Iter first, Iter last, Pred pred = {})
{
    return std::unique(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename Pred = std::equal_to<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectRelation<Pred, iterator_t<Range>> &&
                 Permutable<iterator_t<Range>>)>
safe_iterator_t<Range> unique(Range&& range, Pred pred = {})
{
    return std::unique(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

template <typename Iter1, typename Iter2, typename Pred = std::equal_to<>,
        REQUIRES(InputIterator<Iter1> &&
                 WeaklyIncrementable<Iter2> &&
                 IndirectRelation<Pred, Iter1> &&
                 IndirectlyCopyable<Iter1, Iter2> &&
                 (ForwardIterator<Iter1> ||
                     (InputIterator<Iter2> && Same<value_type_t<Iter1>, detail::detected_t<value_type_t, Iter2>>) ||
                      IndirectlyCopyableStorable<Iter1, Iter2>))>
Iter2 unique_copy(Iter1 first, Iter1 last, Iter2 ofirst, Pred pred = {})
{
    return std::unique_copy(std::move(first), std::move(last), std::move(ofirst), std::move(pred));
}

template <typename Range1, typename Iter2, typename Pred = std::equal_to<>,
        REQUIRES(InputRange<Range1> &&
                 WeaklyIncrementable<Iter2> &&
                 IndirectRelation<Pred, iterator_t<Range1>> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter2> &&
                 (ForwardRange<Range1> ||
                      (InputIterator<Iter2> && Same<range_value_type_t<Range1>, detail::detected_t<value_type_t, Iter2>>) ||
                       IndirectlyCopyableStorable<iterator_t<Range1>, Iter2>))>
Iter2 unique_copy(Range1&& range, Iter2 ofirst, Pred pred = {})
{
    return std::unique_copy(nanorange::begin(range), nanorange::end(range), std::move(ofirst), std::move(pred));
}

// 11.4.10 Reverse

template <typename Iter,
          REQUIRES(BidirectionalIterator<Iter> &&
                   Permutable<Iter>)>
void reverse(Iter first, Iter last)
{
    std::reverse(std::move(first), std::move(last));
}

template <typename Range,
          REQUIRES(BidirectionalRange<Range> &&
                   Permutable<iterator_t<Range>>)>
void reverse(Range&& range)
{
    std::reverse(nanorange::begin(range), nanorange::end(range));
}

template <typename Iter1, typename Iter2,
          REQUIRES(BidirectionalIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyCopyable<Iter1, Iter2>)>
Iter2 reverse_copy(Iter1 first, Iter1 last, Iter2 ofirst)
{
    return std::reverse_copy(std::move(first), std::move(last), std::move(ofirst));
}

template <typename Range1, typename Iter2,
          REQUIRES(BidirectionalRange<Range1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 reverse_copy(Range1&& range, Iter2 ofirst)
{
    return std::reverse_copy(nanorange::begin(range), nanorange::end(range),
                             std::move(ofirst));
}

// 11.4.11 Rotate

template <typename Iter,
          REQUIRES(ForwardIterator<Iter> &&
                   Permutable<Iter>)>
Iter rotate(Iter first, Iter middle, Iter last)
{
    return std::rotate(std::move(first), std::move(middle), std::move(last));
}

template <typename Range,
          REQUIRES(ForwardRange<Range> &&
                   Permutable<iterator_t<Range>>)>
safe_iterator_t<Range>
rotate(Range&& range, iterator_t<Range> middle)
{
    return std::rotate(nanorange::begin(range), std::move(middle), nanorange::end(range));
}

template <typename Iter1, typename Iter2,
          REQUIRES(ForwardIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   IndirectlyCopyable<Iter1, Iter2>)>
Iter2 rotate_copy(Iter1 first, Iter1 middle, Iter1 last, Iter2 ofirst)
{
    return std::rotate_copy(std::move(first), std::move(middle),
                            std::move(last), std::move(ofirst));
}

template <typename Range1, typename Iter2,
        REQUIRES(ForwardRange<Range1> &&
                 WeaklyIncrementable<Iter2> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter2>)>
Iter2 rotate_copy(Range1&& range, iterator_t<Range1> middle, Iter2 ofirst)
{
    return std::rotate_copy(nanorange::begin(range), std::move(middle),
                            nanorange::end(range), std::move(ofirst));
}

// 11.4.12 Shuffle

template <typename Iter, typename URNG,
          REQUIRES(RandomAccessIterator<Iter> &&
                   Permutable<Iter> &&
                   // UniformRandomNumberGenerator<std::remove_reference_t<URNG>> &&
                   ConvertibleTo<std::result_of_t<URNG&()>, difference_type_t<Iter>>)>
void shuffle(Iter first, Iter last, URNG&& generator)
{
    std::shuffle(std::move(first), std::move(last), std::forward<URNG>(generator));
}

template <typename Range, typename URNG,
        REQUIRES(RandomAccessRange<Range> &&
                 Permutable<iterator_t<Range>> &&
                 // UniformRandomNumberGenerator<std::remove_reference_t<URNG>> &&
                 ConvertibleTo<std::result_of_t<URNG&()>, range_difference_type_t<Range>>)>
void shuffle(Range&& range, URNG&& generator)
{
    std::shuffle(nanorange::begin(range), nanorange::end(range), std::forward<URNG>(generator));
}

// 11.4.13 Partitions

template <typename Iter, typename Pred,
          REQUIRES(InputIterator<Iter> &&
                   IndirectUnaryPredicate<Pred, Iter>)>
bool is_partitioned(Iter first, Iter last, Pred pred)
{
    return std::is_partitioned(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename Pred,
          REQUIRES(InputRange<Range> &&
                   IndirectUnaryPredicate<Pred, iterator_t<Range>>)>
bool is_partitioned(Range&& range, Pred pred)
{
    return std::is_partitioned(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

template <typename Iter, typename Pred,
          REQUIRES(ForwardIterator<Iter> &&
                   IndirectUnaryPredicate<Pred, Iter> &&
                   Permutable<Iter>)>
Iter partition(Iter first, Iter last, Pred pred)
{
    return std::partition(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename Pred,
          REQUIRES(ForwardRange<Range> &&
                   IndirectUnaryPredicate<Pred, iterator_t<Range>> &&
                   Permutable<iterator_t<Range>>)>
safe_iterator_t<Range> partition(Range&& range, Pred pred)
{
    return std::partition(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

template <typename Iter, typename Pred,
        REQUIRES(BidirectionalIterator<Iter> &&
                 IndirectUnaryPredicate<Pred, Iter> &&
                 Permutable<Iter>)>
Iter stable_partition(Iter first, Iter last, Pred pred)
{
    return std::stable_partition(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename Pred,
        REQUIRES(BidirectionalRange<Range> &&
                 IndirectUnaryPredicate<Pred, iterator_t<Range>> &&
                 Permutable<iterator_t<Range>>)>
safe_iterator_t<Range> stable_partition(Range&& range, Pred pred)
{
    return std::stable_partition(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

template <typename Iter1, typename Iter2, typename Iter3, typename Pred,
          REQUIRES(InputIterator<Iter1> &&
                   WeaklyIncrementable<Iter2> &&
                   WeaklyIncrementable<Iter3> &&
                   IndirectUnaryPredicate<Pred, Iter1> &&
                   IndirectlyCopyable<Iter1, Iter2> &&
                   IndirectlyCopyable<Iter1, Iter3>)>
std::pair<Iter2, Iter3>
partition_copy(Iter1 first, Iter1 last, Iter2 otrue, Iter3 ofalse, Pred pred)
{
    return std::partition_copy(std::move(first), std::move(last),
                               std::move(otrue), std::move(ofalse), std::move(pred));
}

template <typename Range1, typename Iter2, typename Iter3, typename Pred,
        REQUIRES(InputRange<Range1> &&
                 WeaklyIncrementable<Iter2> &&
                 WeaklyIncrementable<Iter3> &&
                 IndirectUnaryPredicate<Pred, iterator_t<Range1>> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter2> &&
                 IndirectlyCopyable<iterator_t<Range1>, Iter3>)>
std::pair<Iter2, Iter3>
partition_copy(Range1&& range, Iter2 otrue, Iter3 ofalse, Pred pred)
{
    return std::partition_copy(nanorange::begin(range), nanorange::end(range),
                               std::move(otrue), std::move(ofalse), std::move(pred));
}


template <typename Iter, typename Pred,
          REQUIRES(ForwardIterator<Iter> &&
                   IndirectUnaryPredicate<Pred, Iter>)>
Iter partition_point(Iter first, Iter last, Pred pred)
{
    return std::partition_point(std::move(first), std::move(last), std::move(pred));
}

template <typename Range, typename Pred,
        REQUIRES(ForwardRange<Range> &&
                 IndirectUnaryPredicate<Pred, iterator_t<Range>>)>
safe_iterator_t<Range> partition_point(Range&& range, Pred pred)
{
    return std::partition_point(nanorange::begin(range), nanorange::end(range), std::move(pred));
}

/*
 * 11.5 Sorting and related operations
 */

// 11.5.1 Sort

// 11.5.1.1 sort

template <typename Iter, typename Comp = std::less<>,
        REQUIRES(RandomAccessIterator<Iter> &&
                 Sortable<Iter, Comp>)>
void sort(Iter first, Iter last, Comp comp = {})
{
    std::sort(std::move(first), std::move(last), std::move(comp));
}

template <typename Range, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<Range> &&
                 Sortable<iterator_t<Range>, Comp>)>
void sort(Range&& range, Comp comp = {})
{
    std::sort(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.1.2 stable_sort

template <typename Iter, typename Comp = std::less<>,
        REQUIRES(RandomAccessIterator<Iter> &&
                 Sortable<Iter, Comp>)>
void stable_sort(Iter first, Iter last, Comp comp = {})
{
    std::stable_sort(std::move(first), std::move(last), std::move(comp));
}

template <typename Range, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<Range> &&
                 Sortable<iterator_t<Range>, Comp>)>
void stable_sort(Range&& range, Comp comp = {})
{
    std::stable_sort(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.1.3 partial_sort

template <typename Iter, typename Comp = std::less<>,
        REQUIRES(RandomAccessIterator<Iter> &&
                 Sortable<Iter, Comp>)>
void partial_sort(Iter first, Iter middle, Iter last, Comp comp = {})
{
    std::partial_sort(std::move(first), std::move(middle), std::move(last), std::move(comp));
}

template <typename Range, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<Range> &&
                 Sortable<iterator_t<Range>, Comp>)>
void partial_sort(Range&& range, iterator_t<Range> middle, Comp comp = {})
{
    std::partial_sort(nanorange::begin(range), std::move(middle), nanorange::end(range), std::move(comp));
}

// 11.5.1.4 partial_sort_copy

template <typename Iter1, typename Iter2, typename Comp = std::less<>,
        REQUIRES(InputIterator<Iter1> &&
                 RandomAccessIterator<Iter2> &&
                 IndirectlyCopyable<Iter1, Iter2> &&
                 Sortable<Iter2, Comp> &&
                 IndirectStrictWeakOrder<Comp, Iter1, Iter2>)>
Iter2 partial_sort_copy(Iter1 first, Iter1 last, Iter2 rfirst, Iter2 rlast, Comp comp = {})
{
    return std::partial_sort_copy(std::move(first), std::move(last),
                                  std::move(rfirst), std::move(rlast),
                                  std::move(comp));
}

template <typename Range1, typename Range2, typename Comp = std::less<>,
        REQUIRES(InputRange<Range1> &&
                 RandomAccessRange<Range2> &&
                 IndirectlyCopyable<iterator_t<Range1>, iterator_t<Range2>> &&
                 Sortable<iterator_t<Range2>, Comp> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<Range1>, iterator_t<Range2>>)>
safe_iterator_t<Range2> partial_sort_copy(Range1&& input, Range2&& result, Comp comp = {})
{
    return std::partial_sort_copy(nanorange::begin(input), nanorange::end(input),
                                  nanorange::begin(result), nanorange::end(result),
                                  std::move(comp));
}

// 11.5.1.5 is_sorted

template <typename Iter, typename Comp = std::less<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectStrictWeakOrder<Comp, Iter>)>
bool is_sorted(Iter first, Iter last, Comp comp = {})
{
    return std::is_sorted(std::move(first), std::move(last), std::move(comp));
}

template <typename Range, typename Comp = std::less<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<Range>>)>
bool is_sorted(Range&& range, Comp comp = {})
{
    return std::is_sorted(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename Iter, typename Comp = std::less<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectStrictWeakOrder<Comp, Iter>)>
Iter is_sorted_until(Iter first, Iter last, Comp comp = {})
{
    return std::is_sorted_until(std::move(first), std::move(last), std::move(comp));
}

template <typename Range, typename Comp = std::less<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<Range>>)>
safe_iterator_t<Range> is_sorted_until(Range&& range, Comp comp = {})
{
    return std::is_sorted_until(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.2 Nth element

template <typename Iter, typename Comp = std::less<>,
        REQUIRES(RandomAccessIterator<Iter> &&
                 Sortable<Iter, Comp>)>
void nth_element(Iter first, Iter nth, Iter last, Comp comp = {})
{
    std::nth_element(std::move(first), std::move(nth), std::move(last), std::move(comp));
}

template <typename Range, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<Range> &&
                 Sortable<iterator_t<Range>, Comp>)>
void nth_element(Range&& range, iterator_t<Range> nth, Comp comp = {})
{
    std::nth_element(nanorange::begin(range), nanorange::end(range), std::move(nth), std::move(comp));
}

// 11.5.3 Binary search operations

// 11.5.3.1 lower_bound

template <typename Iter, typename T, typename Comp = std::less<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectStrictWeakOrder<Comp, const T*, Iter>)>
Iter lower_bound(Iter first, Iter last, const T& value, Comp comp = {})
{
    return std::lower_bound(std::move(first), std::move(last), value, std::move(comp));
}

template <typename Range, typename T, typename Comp = std::less<>,
          REQUIRES(ForwardRange<Range> &&
                   IndirectStrictWeakOrder<Comp, const T*, iterator_t<Range>>)>
safe_iterator_t<Range>
lower_bound(Range&& range, const T& value, Comp comp = {})
{
    return std::lower_bound(nanorange::begin(range), nanorange::end(range), value, std::move(comp));
}

// 11.5.3.2 upper_bound

template <typename Iter, typename T, typename Comp = std::less<>,
          REQUIRES(ForwardIterator<Iter> &&
                   IndirectStrictWeakOrder<Comp, const T*, Iter>)>
Iter upper_bound(Iter first, Iter last, const T& value, Comp comp = {})
{
    return std::upper_bound(std::move(first), std::move(last), value, std::move(comp));
}

template <typename Range, typename T, typename Comp = std::less<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectStrictWeakOrder<Comp, const T*, iterator_t<Range>>)>
safe_iterator_t<Range>
upper_bound(Range&& range, const T& value, Comp comp = {})
{
    return std::upper_bound(nanorange::begin(range), nanorange::end(range), value, std::move(comp));
}

// 11.5.3.3 equal_range

template <typename Iter, typename T, typename Comp = std::less<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectStrictWeakOrder<Comp, const T*, Iter>)>
std::pair<Iter, Iter>
equal_range(Iter first, Iter last, const T& value, Comp comp = {})
{
    return std::equal_range(std::move(first), std::move(last), value, std::move(comp));
}

template <typename Range, typename T, typename Comp = std::less<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectStrictWeakOrder<Comp, const T*, iterator_t<Range>>)>
std::pair<safe_iterator_t<Range>, safe_iterator_t<Range>>
equal_range(Range&& range, const T& value, Comp comp = {})
{
    return std::equal_range(nanorange::begin(range), nanorange::end(range), value, std::move(comp));
}

// 11.5.3.4 binary_search

template <typename Iter, typename T, typename Comp = std::less<>,
        REQUIRES(ForwardIterator<Iter> &&
                 IndirectStrictWeakOrder<Comp, const T*, Iter>)>
bool binary_search(Iter first, Iter last, const T& value, Comp comp = {})
{
    return std::binary_search(std::move(first), std::move(last), value, std::move(comp));
}

template <typename Range, typename T, typename Comp = std::less<>,
        REQUIRES(ForwardRange<Range> &&
                 IndirectStrictWeakOrder<Comp, const T*, iterator_t<Range>>)>
bool binary_search(Range&& range, const T& value, Comp comp = {})
{
    return std::binary_search(nanorange::begin(range), nanorange::end(range), value, std::move(comp));
}

// 11.5.4 Merge

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Comp = std::less<>,
        REQUIRES(InputIterator<InputIt1> &&
                 InputIterator<InputIt2> &&
                 WeaklyIncrementable<OutputIt> &&
                 Mergeable<InputIt1, InputIt2, OutputIt, Comp>)>
OutputIt merge(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,  OutputIt ofirst, Comp comp = {})
{
    return std::merge(std::move(first1), std::move(last1),
                      std::move(first2), std::move(last2),
                      std::move(ofirst), std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename OutputIt, typename Comp = std::less<>,
          REQUIRES(InputRange<InputRng1> &&
                   InputRange<InputRng2> &&
                   WeaklyIncrementable<OutputIt> &&
                   Mergeable<iterator_t<InputRng1>, iterator_t<InputRng2>, OutputIt, Comp>)>
OutputIt merge(InputRng1&& range1, InputRng2&& range2, OutputIt ofirst, Comp comp = {})
{
    return std::merge(nanorange::begin(range1), nanorange::end(range1),
                      nanorange::begin(range2), nanorange::end(range2),
                      std::move(ofirst), std::move(comp));
}

template <typename BidirIt, typename Comp = std::less<>,
        REQUIRES(BidirectionalIterator<BidirIt> &&
                 Sortable<BidirIt, Comp>)>
void inplace_merge(BidirIt first, BidirIt middle, BidirIt last, Comp comp = {})
{
    std::inplace_merge(std::move(first), std::move(middle), std::move(last), std::move(comp));
}

template <typename BidirRng, typename Comp = std::less<>,
        REQUIRES(BidirectionalRange<BidirRng> &&
                 Sortable<iterator_t<BidirRng>, Comp>)>
void inplace_merge(BidirRng&& range, iterator_t<BidirRng> middle, Comp comp = {})
{
    std::inplace_merge(nanorange::begin(range), std::move(middle), nanorange::end(range), std::move(comp));
}

// 11.5.5. Set operations on sorted structures

// 11.5.5.1 includes

template <typename InputIt1, typename InputIt2, typename Comp = std::less<>,
          REQUIRES(InputIterator<InputIt1> &&
                   InputIterator<InputIt2> &&
                   IndirectStrictWeakOrder<Comp, InputIt1, InputIt2>)>
bool includes(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Comp comp = {})
{
    return std::includes(std::move(first1), std::move(last1),
                         std::move(first2), std::move(last2),
                         std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename Comp = std::less<>,
        REQUIRES(InputRange<InputRng1> &&
                 InputRange<InputRng2> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<InputRng1>, iterator_t<InputRng2>>)>
bool includes(InputRng1&& range1, InputRng2&& range2, Comp comp = {})
{
    return std::includes(nanorange::begin(range1), nanorange::end(range1),
                         nanorange::begin(range2), nanorange::end(range2),
                         std::move(comp));
}

// 11.5.5.2 set_union

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Comp = std::less<>,
        REQUIRES(InputIterator<InputIt1> &&
                 InputIterator<InputIt2> &&
                 WeaklyIncrementable<OutputIt> &&
                 Mergeable<InputIt1, InputIt2, OutputIt, Comp>)>
OutputIt set_union(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                   OutputIt ofirst, Comp comp = {})
{
    return std::set_union(std::move(first1), std::move(last1),
                          std::move(first2), std::move(last2),
                          std::move(ofirst), std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename OutputIt, typename Comp = std::less<>,
        REQUIRES(InputRange<InputRng1> &&
                 InputRange<InputRng2> &&
                 WeaklyIncrementable<OutputIt> &&
                 Mergeable<iterator_t<InputRng1>, iterator_t<InputRng2>, OutputIt, Comp>)>
OutputIt set_union(InputRng1&& range1, InputRng2&& range2, OutputIt ofirst, Comp comp = {})
{
    return std::set_union(nanorange::begin(range1), nanorange::end(range1),
                          nanorange::begin(range2), nanorange::end(range2),
                          std::move(ofirst), std::move(comp));
}

// 11.5.5.3 set_intersection

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Comp = std::less<>,
          REQUIRES(InputIterator<InputIt1> &&
                   InputIterator<InputIt2> &&
                   WeaklyIncrementable<OutputIt> &&
                   Mergeable<InputIt1, InputIt2, OutputIt, Comp>)>
OutputIt set_intersection(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                          OutputIt ofirst, Comp comp = {})
{
    return std::set_intersection(std::move(first1), std::move(last1),
                                 std::move(first2), std::move(last2),
                                 std::move(ofirst), std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename OutputIt, typename Comp = std::less<>,
          REQUIRES(InputRange<InputRng1> &&
                   InputRange<InputRng2> &&
                   WeaklyIncrementable<OutputIt> &&
                   Mergeable<iterator_t<InputRng1>, iterator_t<InputRng2>, OutputIt, Comp>)>
OutputIt set_intersection(InputRng1&& range1, InputRng2&& range2, OutputIt ofirst, Comp comp = {})
{
    return std::set_intersection(nanorange::begin(range1), nanorange::end(range1),
                                 nanorange::begin(range2), nanorange::end(range2),
                                 std::move(ofirst), std::move(comp));
}

// 11.5.5.4 set_difference

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Comp = std::less<>,
          REQUIRES(InputIterator<InputIt1> &&
                   InputIterator<InputIt2> &&
                   WeaklyIncrementable<OutputIt> &&
                   Mergeable<InputIt1, InputIt2, OutputIt, Comp>)>
OutputIt set_difference(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                        OutputIt ofirst, Comp comp = {})
{
    return std::set_difference(std::move(first1), std::move(last1),
                               std::move(first2), std::move(last2),
                               std::move(ofirst), std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename OutputIt, typename Comp = std::less<>,
        REQUIRES(InputRange<InputRng1> &&
                         InputRange<InputRng2> &&
                         WeaklyIncrementable<OutputIt> &&
                         Mergeable<iterator_t<InputRng1>, iterator_t<InputRng2>, OutputIt, Comp>)>
OutputIt set_difference(InputRng1&& range1, InputRng2&& range2, OutputIt ofirst, Comp comp = {})
{
    return std::set_difference(nanorange::begin(range1), nanorange::end(range1),
                               nanorange::begin(range2), nanorange::end(range2),
                               std::move(ofirst), std::move(comp));
}

// 11.5.5.5 set_symmetric_difference

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Comp = std::less<>,
          REQUIRES(InputIterator<InputIt1> &&
                   InputIterator<InputIt2> &&
                   WeaklyIncrementable<OutputIt> &&
                   Mergeable<InputIt1, InputIt2, OutputIt, Comp>)>
OutputIt set_symmetric_difference(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                        OutputIt ofirst, Comp comp = {})
{
    return std::set_symmetric_difference(std::move(first1), std::move(last1),
                                         std::move(first2), std::move(last2),
                                         std::move(ofirst), std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename OutputIt, typename Comp = std::less<>,
        REQUIRES(InputRange<InputRng1> &&
                 InputRange<InputRng2> &&
                 WeaklyIncrementable<OutputIt> &&
                 Mergeable<iterator_t<InputRng1>, iterator_t<InputRng2>, OutputIt, Comp>)>
OutputIt set_symmetric_difference(InputRng1&& range1, InputRng2&& range2, OutputIt ofirst, Comp comp = {})
{
    return std::set_symmetric_difference(nanorange::begin(range1), nanorange::end(range1),
                                         nanorange::begin(range2), nanorange::end(range2),
                                         std::move(ofirst), std::move(comp));
}

// 11.5.6 Heap operations

// 11.5.5.1 push_heap

template <typename RandomIt, typename Comp = std::less<>,
        REQUIRES(RandomAccessIterator<RandomIt> &&
                 Sortable<RandomIt, Comp>)>
void push_heap(RandomIt first, RandomIt last, Comp comp = {})
{
    std::push_heap(std::move(first), std::move(last), std::move(comp));
}

template <typename RandomRng, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<RandomRng> &&
                 Sortable<iterator_t<RandomRng>, Comp>)>
void push_heap(RandomRng&& range, Comp comp = {})
{
    std::push_heap(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.5.2 pop_heap

template <typename RandomIt, typename Comp = std::less<>,
          REQUIRES(RandomAccessIterator<RandomIt> &&
                   Sortable<RandomIt, Comp>)>
void pop_heap(RandomIt first, RandomIt last, Comp comp = {})
{
    std::pop_heap(std::move(first), std::move(last), std::move(comp));
}

template <typename RandomRng, typename Comp = std::less<>,
          REQUIRES(RandomAccessRange<RandomRng> &&
                   Sortable<iterator_t<RandomRng>, Comp>)>
void pop_heap(RandomRng&& range, Comp comp = {})
{
    std::pop_heap(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.5.3 make_heap

template <typename RandomIt, typename Comp = std::less<>,
          REQUIRES(RandomAccessIterator<RandomIt> &&
                   Sortable<RandomIt, Comp>)>
void make_heap(RandomIt first, RandomIt last, Comp comp = {})
{
    std::make_heap(std::move(first), std::move(last), std::move(comp));
}

template <typename RandomRng, typename Comp = std::less<>,
          REQUIRES(RandomAccessRange<RandomRng> &&
                   Sortable<iterator_t<RandomRng>, Comp>)>
void make_heap(RandomRng&& range, Comp comp = {})
{
    std::make_heap(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.6.4 sort_heap

template <typename RandomIt, typename Comp = std::less<>,
          REQUIRES(RandomAccessIterator<RandomIt> &&
                   Sortable<RandomIt, Comp>)>
void sort_heap(RandomIt first, RandomIt last, Comp comp = {})
{
    std::sort_heap(std::move(first), std::move(last), std::move(comp));
}

template <typename RandomRng, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<RandomRng> &&
                 Sortable<iterator_t<RandomRng>, Comp>)>
void sort_heap(RandomRng&& range, Comp comp = {})
{
    std::sort_heap(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.6.5 is_heap

template <typename RandomIt, typename Comp = std::less<>,
          REQUIRES(RandomAccessIterator<RandomIt> &&
                   IndirectStrictWeakOrder<Comp, RandomIt>)>
bool is_heap(RandomIt first, RandomIt last, Comp comp = {})
{
    return std::is_heap(std::move(first), std::move(last), std::move(comp));
}

template <typename RandomRng, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<RandomRng> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<RandomRng>>)>
bool is_heap(RandomRng&& range, Comp comp = {})
{
    return std::is_heap(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename RandomIt, typename Comp = std::less<>,
        REQUIRES(RandomAccessIterator<RandomIt> &&
                  IndirectStrictWeakOrder<Comp, RandomIt>)>
RandomIt is_heap_until(RandomIt first, RandomIt last, Comp comp = {})
{
    return std::is_heap_until(std::move(first), std::move(last), std::move(comp));
}

template <typename RandomRng, typename Comp = std::less<>,
        REQUIRES(RandomAccessRange<RandomRng> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<RandomRng>>)>
safe_iterator_t<RandomRng> is_heap_until(RandomRng&& range, Comp comp = {})
{
    return std::is_heap_until(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.7 Minimum and maximum

template <typename T, typename Comp = std::less<>,
          REQUIRES(IndirectStrictWeakOrder<Comp, const T*>)>
constexpr const T& min(const T& a, const T& b, Comp comp = {})
{
    return std::min(a, b, std::move(comp));
}

template <typename T, typename Comp = std::less<>,
        REQUIRES(Copyable<T> &&
                 IndirectStrictWeakOrder<Comp, const T*>)>
constexpr T min(std::initializer_list<T> ilist, Comp comp = {})
{
    return std::min(ilist, std::move(comp));
}

template <typename ForwardRng, typename Comp = std::less<>,
          REQUIRES(ForwardRange<ForwardRng> &&
                   IndirectStrictWeakOrder<Comp, iterator_t<ForwardRng>> &&
                   Copyable<value_type_t<iterator_t<ForwardRng>>>)>
constexpr value_type_t<iterator_t<ForwardRng>>
min(ForwardRng&& range, Comp comp = {})
{
    return *std::min_element(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename T, typename Comp = std::less<>,
          REQUIRES(IndirectStrictWeakOrder<Comp, const T*>)>
constexpr const T& max(const T& a, const T& b, Comp comp = {})
{
    return std::max(a, b, std::move(comp));
}

template <typename T, typename Comp = std::less<>,
          REQUIRES(Copyable<T> &&
                   IndirectStrictWeakOrder<Comp, const T*>)>
constexpr T max(std::initializer_list<T> ilist, Comp comp = {})
{
    return std::max(ilist, std::move(comp));
}

template <typename ForwardRng, typename Comp = std::less<>,
          REQUIRES(ForwardRange<ForwardRng> &&
                   Copyable<value_type_t<iterator_t<ForwardRng>>> &&
                   IndirectStrictWeakOrder<Comp, iterator_t<ForwardRng>>)>
value_type_t<iterator_t<ForwardRng>>
max(ForwardRng&& range, Comp comp = {})
{
    return *std::max_element(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename T, typename Comp = std::less<>,
          REQUIRES(IndirectStrictWeakOrder<Comp, const T*>)>
constexpr std::pair<const T&, const T&>
minmax(const T& a, const T& b, Comp comp)
{
    return std::minmax(a, b, std::move(comp));
}

template <typename T, typename Comp = std::less<>,
          REQUIRES(Copyable<T> &&
                   IndirectStrictWeakOrder<Comp, const T*>)>
constexpr std::pair<T, T>
minmax(std::initializer_list<T> ilist, Comp comp)
{
    return std::minmax(ilist, std::move(comp));
}

template <typename ForwardRng, typename Comp = std::less<>,
        REQUIRES(Copyable<value_type_t<iterator_t<ForwardRng>>> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<ForwardRng>>)>
std::pair<range_value_type_t<ForwardRng>, range_value_type_t<ForwardRng>>
minmax(ForwardRng&& range, Comp comp = {})
{
    const auto p = std::minmax_element(nanorange::begin(range), nanorange::end(range), std::move(comp));
    return {*p.first, *p.second};
}

template <typename ForwardIt, typename Comp = std::less<>,
          REQUIRES(ForwardIterator<ForwardIt> &&
                   IndirectStrictWeakOrder<Comp, ForwardIt>)>
ForwardIt min_element(ForwardIt first, ForwardIt last, Comp comp = {})
{
    return std::min_element(std::move(first), std::move(last), std::move(comp));
}

template <typename ForwardRng, typename Comp = std::less<>,
        REQUIRES(ForwardRange<ForwardRng> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<ForwardRng>>)>
safe_iterator_t<ForwardRng>
min_element(ForwardRng&& range, Comp comp = {})
{
    return std::min_element(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename ForwardIt, typename Comp = std::less<>,
          REQUIRES(ForwardIterator<ForwardIt> &&
                   IndirectStrictWeakOrder<Comp, ForwardIt>)>
ForwardIt max_element(ForwardIt first, ForwardIt last, Comp comp = {})
{
    return std::max_element(std::move(first), std::move(last), std::move(comp));
}

template <typename ForwardRng, typename Comp = std::less<>,
        REQUIRES(ForwardRange<ForwardRng> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<ForwardRng>>)>
safe_iterator_t<ForwardRng>
max_element(ForwardRng&& range, Comp comp = {})
{
    return std::max_element(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename ForwardIt, typename Comp = std::less<>,
          REQUIRES(ForwardIterator<ForwardIt> &&
                   IndirectStrictWeakOrder<Comp, ForwardIt>)>
constexpr std::pair<ForwardIt, ForwardIt>
minmax_element(ForwardIt first, ForwardIt last, Comp comp = {})
{
    return std::minmax_element(std::move(first), std::move(last), std::move(comp));
}

template <typename ForwardRng, typename Comp = std::less<>,
          REQUIRES(ForwardRange<ForwardRng> &&
                   IndirectStrictWeakOrder<Comp, iterator_t<ForwardRng>>)>
std::pair<safe_iterator_t<ForwardRng>, safe_iterator_t<ForwardRng>>
minmax_element(ForwardRng&& range, Comp comp = {})
{
    return std::minmax_element(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

// 11.5.8 Lexicographical operations

template <typename InputIt1, typename InputIt2, typename Comp = std::less<>,
          REQUIRES(InputIterator<InputIt1> &&
                   InputIterator<InputIt2> &&
                   IndirectStrictWeakOrder<Comp, InputIt1, InputIt2>)>
bool lexicographical_compare(InputIt1 first1, InputIt2 last1,
                             InputIt2 first2, InputIt2 last2, Comp comp = {})
{
    return std::lexicographical_compare(std::move(first1), std::move(last1),
                                        std::move(first2), std::move(last2),
                                        std::move(comp));
}

template <typename InputRng1, typename InputRng2, typename Comp = std::less<>,
        REQUIRES(InputRange<InputRng1> &&
                 InputRange<InputRng2> &&
                 IndirectStrictWeakOrder<Comp, iterator_t<InputRng1>, iterator_t<InputRng2>>)>
bool lexicographical_compare(InputRng1&& range1, InputRng2&& range2, Comp comp = {})
{
    return std::lexicographical_compare(nanorange::begin(range1), nanorange::end(range1),
                                        nanorange::begin(range2), nanorange::end(range2),
                                        std::move(comp));
}

// 11.5.9 Permutation generators

template <typename BidirIt, typename Comp = std::less<>,
          REQUIRES(BidirectionalIterator<BidirIt> &&
                   Sortable<BidirIt, Comp>)>
bool next_permutation(BidirIt first, BidirIt last, Comp comp = {})
{
    return std::next_permutation(std::move(first), std::move(last), std::move(comp));
}

template <typename BidirRng, typename Comp = std::less<>,
        REQUIRES(BidirectionalRange<BidirRng> &&
                 Sortable<iterator_t<iterator_t<BidirRng>>, Comp>)>
bool next_permutation(BidirRng&& range, Comp comp = {})
{
    return std::next_permutation(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

template <typename BidirIt, typename Comp = std::less<>,
        REQUIRES(BidirectionalIterator<BidirIt> &&
                 Sortable<BidirIt, Comp>)>
bool prev_permutation(BidirIt first, BidirIt last, Comp comp = {})
{
    return std::prev_permutation(std::move(first), std::move(last), std::move(comp));
}

template <typename BidirRng, typename Comp = std::less<>,
        REQUIRES(BidirectionalRange<BidirRng> &&
                 Sortable<iterator_t<BidirRng>, Comp>)>
bool prev_permutation(BidirRng&& range, Comp comp = {})
{
    return std::prev_permutation(nanorange::begin(range), nanorange::end(range), std::move(comp));
}

/*
 * Numeric algorithms
 */

// N.B The Ranges TS does not specify constrained versions of these
// functions. What follows is a pure extension, with my (probably incorrect)
// guess about what the constraints should be

template <typename ForwardIt, typename T,
          REQUIRES(ForwardIterator<ForwardIt> &&
                   Writable<ForwardIt, const T&> &&
                   WeaklyIncrementable<T>)>
void iota(ForwardIt first, ForwardIt last, T value)
{
    return std::iota(std::move(first), std::move(last), std::move(value));
}

template <typename ForwardRng, typename T,
          REQUIRES(ForwardRange<ForwardRng> &&
                   Writable<iterator_t<ForwardRng>, T>)>
void iota(ForwardRng&& range, T value)
{
    return std::iota(nanorange::begin(range), nanorange::end(range), std::move(value));
}

template <typename InputIt, typename T, typename BinOp = std::plus<>,
          REQUIRES(InputIterator<InputIt> &&
                   Copyable<T> &&
                   Assignable<T, std::result_of_t<BinOp&(const T&, value_type_t<InputIt>)>>)>
T accumulate(InputIt first, InputIt last, T init, BinOp op = {})
{
    return std::accumulate(std::move(first), std::move(last), std::move(init), std::move(op));
}

template <typename InputRng, typename T, typename BinOp = std::plus<>,
        REQUIRES(InputRange<InputRng> &&
                std::is_assignable<T,
                         std::result_of_t<BinOp&(const T&, range_value_type_t<InputRng>)>>::value)>
T accumulate(InputRng&& range, T init, BinOp op = {})
{
    return std::accumulate(nanorange::begin(range), nanorange::end(range), std::move(init), std::move(op));
}

// Oh boy
template <typename InputIt1, typename InputIt2, typename T,
          typename BinOp1 = std::plus<>, typename BinOp2 = std::multiplies<>,
          REQUIRES(InputIterator<InputIt1> &&
                   InputIterator<InputIt2> &&
                   std::is_assignable<T, std::result_of_t<
                       BinOp1&(T, std::result_of_t<
                           BinOp2&(value_type_t<InputIt1>, value_type_t<InputIt2>)>)>>::value)>
NANORANGE_DEPRECATED
T inner_product(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                T value, BinOp1 op1 = {}, BinOp2 op2 = {})
{
    return std::inner_product(std::move(first1), std::move(last1), std::move(first2),
                              std::move(value), std::move(op1), std::move(op2));
}

template <typename InputIt1, typename InputIt2, typename T,
        typename BinOp1 = std::plus<>, typename BinOp2 = std::multiplies<>,
        REQUIRES(InputIterator<InputIt1> &&
                         InputIterator<InputIt2> &&
                                 std::is_assignable<T, std::result_of_t<
                                 BinOp1&(T, std::result_of_t<
                         BinOp2&(value_type_t<InputIt1>, value_type_t<InputIt2>)>)>>::value)>
T inner_product(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2,
                T value, BinOp1 op1 = {}, BinOp2 op2 = {})
{
    // Thanks cppreference
    while (first1 != last1 && first2 != last2) {
        value = op1(value, op2(*first1, *first2));
        ++first1;
        ++first2;
    }
    return value;
}

template <typename InputRng1, typename InputRng2, typename T,
        typename BinOp1 = std::plus<>, typename BinOp2 = std::multiplies<>,
        REQUIRES(InputRange<InputRng1> &&
                         InputRange<InputRng2> &&
                                 std::is_assignable<T, std::result_of_t<
                                 BinOp1&(T, std::result_of_t<
                         BinOp2&(range_value_type_t<InputRng1>, range_value_type_t<InputRng2>)>)>>::value)>
T inner_product(InputRng1&& range1, InputRng2&& range2,
                T value, BinOp1 op1 = {}, BinOp2 op2 = {})
{
    return nanorange::inner_product(
                 nanorange::begin(range1), nanorange::end(range1),
                 nanorange::begin(range2), nanorange::end(range2),
                 std::move(value), std::move(op1), std::move(op2));
}

template <typename InputIt,  typename OutputIt, typename BinOp = std::minus<>,
          REQUIRES(InputIterator<InputIt> &&
                   OutputIterator<OutputIt,
                        std::result_of_t<BinOp&(value_type_t<InputIt>, value_type_t<InputIt>)>>)>
OutputIt adjacent_difference(InputIt first, InputIt last, OutputIt ofirst, BinOp op = {})
{
    return std::adjacent_difference(std::move(first), std::move(last), std::move(ofirst), std::move(op));
}

template <typename InputRng,  typename OutputIt, typename BinOp = std::minus<>,
        REQUIRES(InputRange<InputRng> &&
                OutputIterator<OutputIt,
                         std::result_of_t<BinOp&(range_value_type_t<InputRng>, range_value_type_t<InputRng>)>>)>
OutputIt adjacent_difference(InputRng&& range, OutputIt ofirst, BinOp op = {})
{
    return std::adjacent_difference(nanorange::begin(range), nanorange::end(range), std::move(ofirst), std::move(op));
}

template <typename InputIt, typename OutputIt, typename BinOp = std::plus<>,
          REQUIRES(InputIterator<InputIt> &&
                   OutputIterator<OutputIt,
                       std::result_of_t<BinOp&(value_type_t<InputIt>, value_type_t<InputIt>)>>)>
OutputIt partial_sum(InputIt first, InputIt last, OutputIt ofirst, BinOp op = {})
{
    return std::partial_sum(std::move(first), std::move(last), std::move(ofirst), std::move(op));
}

template <typename InputRng, typename OutputIt, typename BinOp = std::plus<>,
        REQUIRES(InputRange<InputRng> &&
                OutputIterator<OutputIt,
                         std::result_of_t<BinOp&(range_value_type_t<InputRng>, range_value_type_t<InputRng>)>>)>
OutputIt partial_sum(InputRng&& range, OutputIt ofirst, BinOp op = {})
{
    return std::partial_sum(nanorange::begin(range), nanorange::end(range), std::move(ofirst), std::move(op));
}

#undef CONCEPT
#undef REQUIRES

} // end namespace nanorange

#endif

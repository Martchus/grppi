/*
 * Copyright 2018 Universidad Carlos III de Madrid
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GRPPI_COMMON_ITERATOR_H
#define GRPPI_COMMON_ITERATOR_H

#include <utility>
#include <tuple>

namespace grppi{

namespace internal {

template <typename F, typename ... Iterators, template<typename...> class T, std::size_t ... I>
decltype(auto) apply_deref_increment(
    F && f, 
    T<Iterators...> & iterators, 
    std::index_sequence<I...>)
{
  return std::forward<F>(f)(*std::get<I>(iterators)++...);
}

} // namespace internal

/**
\brief Applies a callable object to the values obtained from the iterators in 
a tuple-like object.
This function takes callable object `f` and a tuple-like with iterators (e.g.
the result of `make_tuple(it1, it2, it3)`)

and performs the action

~~~{.cpp}
f(*it1++, *it2++, *it3++);
~~~

\tparam F Type of the callable object.
\tparam Iterators Pack of iterator types.
\tparam T Tuple-like type containing the iterators
\param f Callable object to be invoked.
\param iterators Iterators to be used.
\post All iterators in t have been incremented
\return The result of callable invocation.
*/
template <typename F, typename ... Iterators, template <typename ...> class T>
decltype(auto) apply_deref_increment(
    F && f, 
    T<Iterators...> & iterators)
{
  constexpr std::size_t size = sizeof...(Iterators);
  return internal::apply_deref_increment(std::forward<F>(f), iterators,
      std::make_index_sequence<size>());
}

namespace internal {

template <typename F, typename ... Iterators, template <typename...> class T,
          std::size_t ... I>
decltype(auto) apply_increment(
    F && f,
    T<Iterators...> & iterators,
    std::index_sequence<I...>)
{
  return std::forward<F>(f)(std::get<I>(iterators)++...);
}

}

/**
\brief Applies a callable object to the iterators in a tuple like-object and
the increments those iterators.
This function takes callable object `f` and a tuple-like object with iterators (e.g.
the result of `make_tuple(it1, it2, it3)`)

and performs the action

~~~{.cpp}
f(it1++, it2++, it3++);
~~~

\tparam Type of the callable object.
\tparam Iterators Pack of iterator types.
\tparam T Tuple-like type containing the iterators
\param f Callable object to be invoked.
\param iterators Iterators to be used.
\post All iterators in t have been incremented
*/
template <typename F, typename ... Iterators, template <typename...> class T>
decltype(auto) apply_increment(
    F && f,
    T<Iterators...> & iterators) 
{
  constexpr std::size_t size = sizeof...(Iterators);
  return internal::apply_increment(std::forward<F>(f), iterators,
      std::make_index_sequence<size>());
}

namespace internal {

template <typename F, typename T, std::size_t ... I>
decltype(auto) apply_iterators_indexed_impl(F && f, T && t, std::size_t i,
    std::index_sequence<I...>)
{
  return std::forward<F>(f)(std::get<I>(t)[i]...);
}

} // namespace internal

/**
\brief Applies a callable object to the values obtained from the iterators in a tuple
by indexing.
This function takes callable object `f`, a tuple-like with iterators (e.g.
the result of `make_tuple(it1, it2, it3)`) and an integral index `i`.

and performs the action

~~~{.cpp}
f(it1[i], it2[i], it3[i]);
~~~

\tparam F Type of the callable object.
\tparam T Tuple type containing a tuple of iterators
\param f Callable object to be invoked.
\param t Tuple of iterators.
\param i Integral index to apply to each iterator.
\post All iterators in t have been incremented
\post `f` has been invoked with the contents of the iterator in the tuple.
*/
template <typename F, typename T>
decltype(auto) apply_iterators_indexed(F && f, T && t, std::size_t i)
{
  using tuple_raw_type = std::decay_t<T>;
  constexpr std::size_t size = std::tuple_size<tuple_raw_type>::value;
  return internal::apply_iterators_indexed_impl(std::forward<F>(f), 
      std::forward<T>(t), i, std::make_index_sequence<size>());
}

namespace internal {

template <typename T, std::size_t ... I>
auto iterators_next_impl(T && t, int n, std::index_sequence<I...>) {
  return make_tuple(
    std::next(std::get<I>(t), n)...
  );
}

} // namespace internal

/**
\brief Computes next n steps from a tuple of iterators.
\tparam T Tuple type containing a tuple of iterators.
\param t Tuple of iterators.
\param n Number of steps to advance.
\note This function is the equivalent to std::next for a tuple of iterators.
\returns A new tuple with the result iterators.
*/
template <typename T>
auto iterators_next(T && t, int n) {
  using tuple_raw_type = std::decay_t<T>;
  constexpr std::size_t size = std::tuple_size<tuple_raw_type>::value;
  return internal::iterators_next_impl(std::forward<T>(t), n,
      std::make_index_sequence<size>());
}




/// Advance a pack of iterators by a delta.
/// Every iterator in the parameter pack in is increased n steps.
template <typename ... InputIt>
void advance_iterators(size_t delta, InputIt & ... in) {
  // This hack can be done in C++14.
  // It can be simplified in C++17 with folding expressions.
  using type = int[];
  type { 0, (in += delta, 0) ...};
} 

/// Advance a pack of iterators by one unit.
/// Every iterator in the parameter pack in is increased 1 step.
template <typename ... InputIt>
void advance_iterators(InputIt & ... in) {
  // This hack can be done in C++14.
  // It can be simplified in C++17 with folding expressions.
  using type = int[];
  type { 0, (in++, 0) ...};
} 


} // namespace grppi

#endif

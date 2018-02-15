/**
* @version		GrPPI v0.3
* @copyright		Copyright (C) 2017 Universidad Carlos III de Madrid. All rights reserved.
* @license		GNU/GPL, see LICENSE.txt
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
* You have received a copy of the GNU General Public License in LICENSE.txt
* also available in <http://www.gnu.org/licenses/gpl.html>.
*
* See COPYRIGHT.txt for copyright notices and details.
*/

#ifndef GRPPI_FF_PARALLEL_EXECUTION_FF_H
#define GRPPI_FF_PARALLEL_EXECUTION_FF_H

#ifdef GRPPI_FF

#include "../common/iterator.h"
#include "../common/execution_traits.h"

#include <type_traits>
#include <tuple>
#include <thread>
#include <experimental/optional>

#include <ff/parallel_for.hpp>

namespace grppi {

/**
\brief FastFlow (FF) parallel execution policy.

This policy uses FF as implementation back-end.
*/
class parallel_execution_ff {

public:
  /** 
  \brief Default construct a FF parallel execution policy.

  Creates an FF parallel execution object.

  The concurrency degree is determined by the platform.
  */
  parallel_execution_ff() noexcept :
      parallel_execution_ff{
        static_cast<int>(std::thread::hardware_concurrency())}
  {}

  /** 
  \brief Constructs an FF parallel execution policy.

  Creates an FF parallel execution object selecting the concurrency degree
  and ordering.
  \param concurrency_degree Number of threads used for parallel algorithms.
  \param order Whether ordered executions is enabled or disabled.
  */
  parallel_execution_ff(int concurrency_degree, bool order = true) noexcept :
      concurrency_degree_{concurrency_degree},
      ordering_{order}
  {
  }

  /**
  \brief Set number of grppi threads.
  */
  void set_concurrency_degree(int degree) noexcept { 
    concurrency_degree_ = degree; 
  }

  /**
  \brief Get number of grppi trheads.
  */
  int concurrency_degree() const noexcept { 
    return concurrency_degree_; 
  }

  /**
  \brief Enable ordering.
  */
  void enable_ordering() noexcept { ordering_=true; }

  /**
  \brief Disable ordering.
  */
  void disable_ordering() noexcept { ordering_=false; }

  /**
  \brief Is execution ordered.
  */
  bool is_ordered() const noexcept { return ordering_; }

  /**
  \brief Applies a trasnformation to multiple sequences leaving the result in
  another sequence using available FF parallelism
  \tparam InputIterators Iterator types for input sequences.
  \tparam OutputIterator Iterator type for the output sequence.
  \tparam Transformer Callable object type for the transformation.
  \param firsts Tuple of iterators to input sequences.
  \param first_out Iterator to the output sequence.
  \param sequence_size Size of the input sequences.
  \param transform_op Transformation callable object.
  \pre For every I iterators in the range 
       `[get<I>(firsts), next(get<I>(firsts),sequence_size))` are valid.
  \pre Iterators in the range `[first_out, next(first_out,sequence_size)]` are valid.
  */
  template <typename ... InputIterators, typename OutputIterator, 
            typename Transformer>
  void map(std::tuple<InputIterators...> firsts,
      OutputIterator first_out, 
      std::size_t sequence_size, Transformer transform_op) const;

  /**
    \brief Applies a reduction to a sequence of data items.
    \tparam InputIterator Iterator type for the input sequence.
    \tparam Identity Type for the identity value.
    \tparam Combiner Callable object type for the combination.
    \param first Iterator to the first element of the sequence.
    \param sequence_size Size of the input sequence.
    \param last Iterator to one past the end of the sequence.
    \param identity Identity value for the reduction.
    \param combine_op Combination callable object.
    \pre Iterators in the range `[first,last)` are valid.
    \return The reduction result.
   */
  template <typename InputIterator, typename Identity, typename Combiner>
  auto reduce(InputIterator first,
      std::size_t sequence_size,
      Identity && identity,
      Combiner && combine_op) const;

  /**
    \brief Applies a map/reduce operation to a sequence of data items.
    \tparam InputIterator Iterator type for the input sequence.
    \tparam Identity Type for the identity value.
    \tparam Transformer Callable object type for the transformation.
    \tparam Combiner Callable object type for the combination.
    \param first Iterator to the first element of the sequence.
    \param sequence_size Size of the input sequence.
    \param identity Identity value for the reduction.
    \param transform_op Transformation callable object.
    \param combine_op Combination callable object.
    \pre Iterators in the range `[first,last)` are valid.
    \return The map/reduce result.
   */
  template <typename ... InputIterators, typename Identity,
  typename Transformer, typename Combiner>
  auto map_reduce(std::tuple<InputIterators...> firsts,
      std::size_t sequence_size,
      Identity && identity,
      Transformer && transform_op,
      Combiner && combine_op) const;

private:

  int concurrency_degree_ = 
    static_cast<int>(std::thread::hardware_concurrency());
  bool ordering_ = true;
};

/**
\brief Metafunction that determines if type E is parallel_execution_ff
\tparam Execution policy type.
*/
template <typename E>
constexpr bool is_parallel_execution_ff() {
  return std::is_same<E, parallel_execution_ff>::value;
}

/**
\brief Determines if an execution policy is supported in the current compilation.
\note Specialization for parallel_execution_ff when GRPPI_FF is enabled.
*/
template <>
constexpr bool is_supported<parallel_execution_ff>() { return true; }

/**
\brief Determines if an execution policy supports the map pattern.
\note Specialization for parallel_execution_ff when GRPPI_FF is enabled.
*/
template <>
constexpr bool supports_map<parallel_execution_ff>() { return true; }

/**
\brief Determines if an execution policy supports the reduce pattern.
\note Specialization for parallel_execution_ff when GRPPI_FF is enabled.
*/
template <>
constexpr bool supports_reduce<parallel_execution_ff>() { return true; }

/**
\brief Determines if an execution policy supports the map-reduce pattern.
\note Specialization for parallel_execution_ff when GRPPI_FF is enabled.
*/
template <>
constexpr bool supports_map_reduce<parallel_execution_ff>() { return true; }


template <typename ... InputIterators, typename OutputIterator, 
          typename Transformer>
void parallel_execution_ff::map(
    std::tuple<InputIterators...> firsts,
    OutputIterator first_out, 
    std::size_t sequence_size, Transformer transform_op) const
{
  ff::ParallelFor pf{concurrency_degree_, true};
  pf.parallel_for(0, sequence_size,
    [=](const long delta) {
      *std::next(first_out, delta) = apply_iterators_indexed(transform_op, firsts, delta);
    }, 
    concurrency_degree_);
}

template <typename InputIterator, typename Identity, typename Combiner>
auto parallel_execution_ff::reduce(InputIterator first,
    std::size_t sequence_size,
    Identity && identity,
    Combiner && combine_op) const 
{
  ff::ParallelForReduce<Identity> pfr{concurrency_degree_, true};
  Identity result{identity};

  pfr.parallel_reduce(result, identity, 0, sequence_size,
      [combine_op,first](long delta, auto & value) {
        value = combine_op(value, *std::next(first,delta));
      }, 
      [&result, combine_op](auto a, auto b) { result = combine_op(a,b); }, 
      concurrency_degree_);

  return result;
}

template <typename ... InputIterators, typename Identity,
          typename Transformer, typename Combiner>
auto parallel_execution_ff::map_reduce(std::tuple<InputIterators...> firsts,
      std::size_t sequence_size,
      Identity && identity,
      Transformer && transform_op,
      Combiner && combine_op) const 
{
  std::vector<Identity> partial_outs(sequence_size);
  map(firsts, partial_outs.begin(), sequence_size, 
      std::forward<Transformer>(transform_op));

  return reduce(partial_outs.begin(), sequence_size, 
      std::forward<Identity>(identity),
      std::forward<Combiner>(combine_op));
}


} // end namespace grppi

#else // GRPPI_FF undefined

namespace grppi {


/// Parallel execution policy.
/// Empty type if  GRPPI_FF disabled.
struct parallel_execution_ff {};

/**
\brief Metafunction that determines if type E is parallel_execution_ff
This metafunction evaluates to false if GRPPI_FF is disabled.
\tparam Execution policy type.
*/
template <typename E>
constexpr bool is_parallel_execution_ff() {
  return false;
}

}

#endif // GRPPI_FF

#endif

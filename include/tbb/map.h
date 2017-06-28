/*
* @version    GrPPI v0.2
* @copyright    Copyright (C) 2017 Universidad Carlos III de Madrid. All rights reserved.
* @license    GNU/GPL, see LICENSE.txt
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

#ifndef GRPPI_MAP_TBB_H
#define GRPPI_MAP_TBB_H

#ifdef GRPPI_TBB

#include <tbb/tbb.h>
namespace grppi{

/**
\addtogroup map_pattern
@{
\addtogroup map_pattern_tbb TBB parallel map pattern.
Implementation of map pattern for TBB parallel back-end.
@{
*/

/**
\brief Invoke [map pattern](@ref map-pattern) on a data sequence with TBB
parallel execution.
\tparam InputIt Iterator type used for input sequence.
\tparam OtuputIt Iterator type used for the output sequence.
\tparam Operation Callable type for the transformation operation.
\param ex Sequential execution policy object
\param first Iterator to the first element in the input sequence.
\param last Iterator to one past the end of the input sequence.
\param first_out Iterator to first elemento of the output sequence.
\param op Transformation operation.
*/
template <typename InputIt, typename OutputIt, typename Operation>
void map(parallel_execution_tbb &ex, 
         InputIt first, InputIt last, 
         OutputIt first_out, 
         Operation && op)
{
  tbb::parallel_for(
    static_cast<std::size_t>(0), 
    static_cast<std::size_t>((last-first)), 
    [&] (std::size_t index){
      auto current = (first_out+index);
      *current = op(*(first+index));
    }
  );   
}

/**
\brief Invoke [map pattern](@ref map-pattern) on a data sequence with TBB
parallel execution.
\tparam InputIt Iterator type used for input sequence.
\tparam OtuputIt Iterator type used for the output sequence.
\tparam Operation Callable type for the transformation operation.
\param ex Sequential execution policy object
\param first Iterator to the first element in the input sequence.
\param last Iterator to one past the end of the input sequence.
\param first_out Iterator to first elemento of the output sequence.
\param op Transformation operation.
\param more_firsts Additional iterators with first elements of additional sequences.
*/
template <typename InputIt, typename OutputIt, 
          typename ... MoreIn, 
          typename Operation>
void map(parallel_execution_tbb & ex, 
         InputIt first, InputIt last, OutputIt first_out, 
         Operation && op, 
         MoreIn ... inputs)
{
  tbb::parallel_for(
    static_cast<std::size_t>(0),
    static_cast<std::size_t>((last-first)), 
    [&] (std::size_t index){
      auto current = (first_out+index);
      *current = op(*(first+index), *(inputs+index)...);
    }
 );   

}

}

#endif

#endif

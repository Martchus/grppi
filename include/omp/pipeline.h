/**
* @version		GrPPI v0.2
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

#ifndef GRPPI_OMP_PIPELINE_H
#define GRPPI_OMP_PIPELINE_H

#ifdef GRPPI_OMP

#include <experimental/optional>

#include <boost/lockfree/spsc_queue.hpp>

#include "parallel_execution_omp.h"

namespace grppi{

//Last stage
template <typename InQueue, typename Consumer>
void pipeline_impl(parallel_execution_omp & ex, InQueue & input_queue, 
                   Consumer && consume_op)
{
  using namespace std;
  using input_type = typename InQueue::value_type;

  if (ex.is_ordered()){
    vector<input_type> elements;
    long current = 0;
    auto item = input_queue.pop( );
    while (item.first) {
      if (current == item.second) {
        consume_op(*item.first);
        current ++;
      } 
      else {
        elements.push_back(item);
      }
      for (auto it=elements.begin(); it!=elements.end(); it++) {
        if (it->second == current) {
          consume_op(*it->first);
              elements.erase(it);
              current++;
              break;
           }
        }
       item = input_queue.pop( );
      }
      while(elements.size()>0){
        for(auto it = elements.begin(); it != elements.end(); it++){
          if(it->second == current) {
            consume_op(*it->first);
            elements.erase(it);
            current++;
            break;
          }
        }
      }
    }
    else {
      auto item = input_queue.pop();
      while (item.first) {
        consume_op(*item.first);
        item = input_queue.pop();
     }
   }
   //End task
}

template <typename Transformer, typename InQueue, typename... MoreTransformers>
void pipeline_impl(parallel_execution_omp & ex, InQueue & input_queue, 
                   filter_info<parallel_execution_omp,Transformer> & filter_obj, 
                   MoreTransformers && ... more_transform_ops) 
{
  using filter_type = filter_info<parallel_execution_omp, Transformer>;

  pipeline_impl(ex,input_queue, std::forward<filter_type>(filter_obj), 
      std::forward<MoreTransformers>(more_transform_ops)...) ;
}

template <typename Transformer, typename InQueue,
          typename... MoreTransformers>
void pipeline_impl_ordered(parallel_execution_omp & ex, 
                           InQueue & input_queue, 
                           filter_info<parallel_execution_omp,Transformer> && filter_obj, 
                           MoreTransformers && ... more_transform_ops)
{
  using namespace std;
  using input_type = typename InQueue::value_type;
  using input_value_type = typename input_type::first_type;
  auto tmp_queue = ex.make_queue<input_type>();

  atomic<int> done_threads{0};
  for(int th = 0; th<filter_obj.exectype.concurrency_degree(); th++) {
    #pragma omp task shared(tmp_queue,filter_obj,input_queue,done_threads)
    {
      auto item{input_queue.pop()};
      while (item.first) {
        if(filter_obj.task(*item.first)) {
          tmp_queue.push(item);
        }
        else {
          tmp_queue.push(make_pair(input_value_type{} ,item.second));
        }
        item = input_queue.pop();
      }
      done_threads++;
      if (done_threads==filter_obj.exectype.concurrency_degree()) {
        tmp_queue.push (make_pair(input_value_type{}, -1));
      }
      else {
        input_queue.push(item);
      }
    }
  }

  auto output_queue = ex.make_queue<input_type>();
  #pragma omp task shared (output_queue,tmp_queue)
  {
    vector<input_type> elements;
    int current = 0;
    long order = 0;
    auto item = tmp_queue.pop();
    for (;;) {
      if (!item.first && item.second == -1) break;
      if (item.second == current) {
        if (item.first) {
          output_queue.push(make_pair(item.first, order++));
        }
        current++;
      }
      else {
        elements.push_back(item);
      }
      for (auto it=elements.begin(); it<elements.end(); it++) {
        if ((*it).second==current) {
          if((*it).first){
            output_queue.push(make_pair((*it).first,order++));
          }
          elements.erase(it);
          current++;
          break;
        }
      }
      item = tmp_queue.pop();
    }
    while (elements.size()>0) {
      for (auto it=elements.begin(); it<elements.end(); it++) {
        if ((*it).second == current) {
          if((*it).first) {
            output_queue.push(make_pair((*it).first,order++));
          }
          elements.erase(it);
          current++;
          break;
        }
      }
    }
    output_queue.push(item);
  }
  pipeline_impl(ex, output_queue, 
      forward<MoreTransformers>(more_transform_ops)...);
  #pragma omp taskwait
}

template <typename Transformer, typename InQueue,typename... MoreTransformers>
void pipeline_impl_unordered(parallel_execution_omp & ex, InQueue & input_queue, 
                             filter_info<parallel_execution_omp, Transformer> && farm_obj, 
                             MoreTransformers && ... more_transform_ops)
{
  using input_type = typename InQueue::value_type;
  using input_value_type = typename input_type::first_type;
  auto output_queue = ex.make_queue<input_type>();

  std::atomic<int> done_threads{0};
  for (int th=0; th<farm_obj.exectype.concurrency_degree(); th++) {
    #pragma omp task shared(output_queue,farm_obj,input_queue,done_threads)
    {
      auto item = input_queue.pop( ) ;
      while (item.first) {
        if (farm_obj.task(*item.first)) {
          output_queue.push(item);
        }
        item = input_queue.pop();
      }
      done_threads++;
      if (done_threads==farm_obj.exectype.concurrency_degree()) {
        output_queue.push(make_pair(input_value_type{}, -1));
      }
      else {
        input_queue.push(item);
      }
    }
  }
  pipeline_impl(ex, output_queue, 
      std::forward<MoreTransformers>(more_transform_ops)...);
  #pragma omp taskwait
}

template <typename Transformer, typename InQueue,typename... MoreTransformers>
void pipeline_impl(parallel_execution_omp & ex, InQueue & input_queue, 
                   filter_info<parallel_execution_omp, Transformer> && filter_obj, 
                   MoreTransformers && ... more_transform_ops) 
{
  using filter_type = filter_info<parallel_execution_omp, Transformer>;

  if (ex.is_ordered()) {
    pipeline_impl_ordered(ex, input_queue, 
        std::forward<filter_type>(filter_obj),
        std::forward<MoreTransformers>(more_transform_ops)...);
  }
  else {
    pipeline_impl_unordered(ex, input_queue,
        std::forward<filter_type>(filter_obj),
        std::forward<MoreTransformers>(more_transform_ops)...);
  }
}


template <typename Transformer, typename InQueue,typename... MoreTransformers>
void pipeline_impl(parallel_execution_omp & ex, InQueue & input_queue, 
                   farm_info<parallel_execution_omp, Transformer> & farm_obj, 
                   MoreTransformers && ... more_transform_ops) 
{
  using farm_type = farm_info<parallel_execution_omp,Transformer>;
  pipeline_impl(ex, input_queue, std::forward<farm_type>(farm_obj), 
      std::forward<MoreTransformers>(more_transform_ops)...) ;
}

template <typename Transformer, typename InQueue,typename... MoreTransformers>
void pipeline_impl(parallel_execution_omp & ex, InQueue & input_queue, 
                   farm_info<parallel_execution_omp, Transformer> && farm_obj, 
                   MoreTransformers && ... sgs ) 
{
  using namespace std;
  using input_type = typename InQueue::value_type;
  using input_value_type = typename input_type::first_type::value_type;
  using result_type = typename result_of<Transformer(input_value_type)>::type;
  using output_value_type = experimental::optional<result_type>;
  using output_type = pair<output_value_type,long>;
 
  auto output_queue = ex.make_queue<output_type>();
  atomic<int> done_threads{0};
  for (int th=0; th<farm_obj.exectype.concurrency_degree(); th++) {
    #pragma omp task shared(done_threads,output_queue,farm_obj,input_queue)
    {
      auto item = input_queue.pop();
      while (item.first) {
        auto out = output_value_type{farm_obj.task(*item.first)};
        output_queue.push(make_pair(out,item.second));
        item = input_queue.pop();
      }
      input_queue.push(item);
      done_threads++;
      if (done_threads==farm_obj.exectype.concurrency_degree()) {
        output_queue.push(make_pair(output_value_type{}, -1));
      }
    }              
  }
  pipeline_impl(ex, output_queue, forward<MoreTransformers>(sgs) ... );
  #pragma omp taskwait
}

//Intermediate stages
template <typename Transformer, typename InQueue,typename ... MoreTransformers>
void pipeline_impl(parallel_execution_omp & ex, InQueue & input_queue, 
                   Transformer && transform_op, 
                   MoreTransformers && ... more_transform_ops) 
{
  using namespace std;
  using input_type = typename InQueue::value_type;
  using input_value_type = typename input_type::first_type::value_type;
  using result_type = typename result_of<Transformer(input_value_type)>::type;
  using output_value_type = experimental::optional<result_type>;
  using output_type = pair<output_value_type,long>;
  auto output_queue = ex.make_queue<output_queue>();

  //Start task
  #pragma omp task shared(transform_op, input_queue, output_queue)
  {
    auto item = input_queue.pop(); 
    while (item.first) {
      auto out = output_value_type{transform_op(*item.first)};
      output_queue.push(make_pair(out, item.second));
      item = input_queue.pop() ;
    }
    output_queue.push(make_pair(output_value_type{}, -1));
  }
  //End task

  pipeline_impl(ex, output_queue, 
      forward<MoreTransformers>(more_transform_ops)...);
}

/**
\addtogroup pipeline_pattern
@{
*/

/**
\addtogroup pipeline_pattern_omp OpenMP parallel pipeline pattern
\brief OpenMP parallel implementation of the \ref md_pipeline pattern
@{
*/

/**
\brief Invoke [pipeline pattern](@ref md_pipeline) on a data stream
with OpenMP parallel execution.
\tparam Generator Callable type for the stream generator.
\tparam Transformers Callable type for each transformation stage.
\param ex OpenMP parallel execution policy object.
\param generate_op Generator operation.
\param trasnform_ops Transformation operations for each stage.
\remark Generator shall be a zero argument callable type.
*/
template <typename Generator, typename ... Transformers,
          requires_no_arguments<Generator> = 0>
void pipeline(parallel_execution_omp & ex, Generator && generate_op, 
              Transformers && ... transform_ops) 
{
  using namespace std;

  using result_type = typename result_of<Generator()>::type;
  auto output_queue = ex.make_queue<pair<result_type,long>>(); 

  #pragma omp parallel
  {
    #pragma omp single nowait
    {
      #pragma omp task shared(generate_op,output_queue)
      {
        long order = 0;
        for (;;) {
          auto item = generate_op();
          output_queue.push(make_pair(item,order++)) ;
          if (!item) break;
        }
      }
      pipeline_impl(ex, output_queue,
          forward<Transformers>(transform_ops)...);
      #pragma omp taskwait
    }
  }
}

/**
@}
@}
*/

}
#endif

#endif

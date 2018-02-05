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

#ifndef GRPPI_WINDOW_H
#define GRPPI_WINDOW_H

#include "common/window_pattern.h"
#include "common/active_window_pattern.h"


namespace grppi {

/** 
\addtogroup stream_building_blocks
@{
\defgroup window_pattern Window pattern
\brief Interface for applyinng the \ref md_window.
@{
*/



/**
\brief Invoke \ref md_window on a data stream 
that can be composed in other streaming patterns.
\tparam Window Type for the window policy.
\param window_policy policy to generate the windows.
*/
template <typename Window>
auto window(Window && window_policy)
{
   return window_t<Window>{std::forward<Window>(window_policy)};
}

template <typename Window>
auto window(Window & window_policy)
{
   return window(std::forward<Window>(window_policy));
}

template <typename Window>
auto active_window(Window && window_policy)
{
   return active_window_t<Window>{std::forward<Window>(window_policy)};
}

template <typename Window>
auto active_window(Window & window_policy)
{
   return active_window(std::forward<Window>(window_policy));
}
/**
@}
@}
*/

}

#endif
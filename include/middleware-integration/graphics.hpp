/* (c) Copyright 2011-2014 Felipe Magno de Almeida
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIDDLEWARE_INTEGRATION_GRAPHICS_HPP
#define MIDDLEWARE_INTEGRATION_GRAPHICS_HPP

#include <middleware-api/graphics.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <deque>

namespace middleware_integration {

struct window
{
  middleware_api_graphics_surface_t surface;
  int zindex;
  int x, y, w, h;

  window(middleware_api_graphics_surface_t surface, int zindex
         , int x, int y, int w, int h)
    : surface(surface), zindex(zindex), x(x), y(y)
    , w(w), h(h) {}
};

inline bool operator==(window lhs, window rhs)
{
  return lhs.surface == rhs.surface;
}
inline bool operator!=(window lhs, window rhs)
{
  return !(lhs == rhs);
}
inline bool operator<(window lhs, window rhs)
{
  return std::less<middleware_api_graphics_surface_t>()(lhs.surface, rhs.surface);
}

struct graphics
{
  graphics() : surface(0), dirty(false)
  {
    surface_mutex.reset(new boost::mutex);
    condition_variable.reset(new boost::condition_variable);
  }
  graphics(middleware_api_graphics_surface_t surface)
    : surface(surface), dirty(false)
  {
    surface_mutex.reset(new boost::mutex);
    condition_variable.reset(new boost::condition_variable);
  }

  window add_shared_surface(::middleware_api_graphics_surface_t surface, int zindex
                            , int x, int y, boost::mutex& mutex);
  window add_surface(::middleware_api_graphics_surface_t surface, int zindex
                     , int x, int y);
  void mark_dirty(window w);
  void mark_dirty_and_wait_redraw(window w);
  void draw_on_primary_surface(::middleware_api_graphics_surface_t surface);

  middleware_api_graphics_surface_t surface;
  std::deque<window> windows;
  bool dirty;
  boost::shared_ptr<boost::mutex> surface_mutex;
  boost::shared_ptr<boost::condition_variable> condition_variable;
};

void register_graphics_to_draw_on_primary_surface(graphics& g);

}

#endif

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

#include <middleware-integration/graphics.hpp>

namespace middleware_integration {

namespace {

void draw_on_primary_surface(middleware_api_graphics_surface_t surface
                             , void* ud)
{
  static_cast<graphics*>(ud)->draw_on_primary_surface(surface);
}

}

void register_graphics_to_draw_on_primary_surface(graphics& g)
{
  std::cout << "register " << &g << std::endl;
  middleware_api_graphics_on_primary_surface_draw(&draw_on_primary_surface, &g);
}

struct zindex_comparator
{
  bool operator()(window lhs, window rhs) const
  {
    return lhs.zindex < rhs.zindex;
  }
};

window graphics::add_shared_surface( ::middleware_api_graphics_surface_t surface, int zindex
                                    , int x, int y, boost::mutex& mutex)
{
  window w(surface, zindex, x, y, middleware_api_graphics_width(surface)
           , middleware_api_graphics_height(surface));
  windows.push_front(w);
  std::stable_sort(windows.begin(), windows.end(), zindex_comparator());
  return w;
}
window graphics::add_surface( ::middleware_api_graphics_surface_t surface, int zindex
                             , int x, int y)
{
  window w(surface, zindex, x, y, middleware_api_graphics_width(surface)
           , middleware_api_graphics_height(surface));
  windows.push_front(w);
  std::stable_sort(windows.begin(), windows.end(), zindex_comparator());
  return w;
}
  
void graphics::mark_dirty(window w)
{
  boost::unique_lock<boost::mutex> l(*surface_mutex);
  dirty = true;
}

void graphics::mark_dirty_and_wait_redraw(window w)
{
  std::cout << "graphics::mark_dirty_and_wait_redraw " << this << std::endl;
  boost::unique_lock<boost::mutex> l(*surface_mutex);
  assert(surface != 0);
  for(std::deque<window>::const_iterator first = windows.begin()
        , last = windows.end(); first != last; ++first)
  {
    std::cout << "drawing window " << first->x << 'x'
              << first->y << ':' << first->w
              << 'x' << first->h << ' '
              << "0x0:" << first->w << ' ' << first->h 
              << std::endl;
    middleware_api_graphics_stretch_bitblit
      (surface, first->surface, first->x
       , first->y, first->w, first->h
       , 0, 0, first->w, first->h);
  }
  std::cout << "drawing on primary surface" << std::endl;
  // middleware_api_graphics_draw_on_primary_surface(surface);
  dirty = true;
  while(dirty)
    condition_variable->wait(l);
}

void graphics::draw_on_primary_surface(::middleware_api_graphics_surface_t surface)
{
  boost::unique_lock<boost::mutex> l(*surface_mutex);
  middleware_api_graphics_stretch_bitblit(surface, this->surface
                                          , 0, 0
                                          , middleware_api_graphics_primary_surface_width()
                                          , middleware_api_graphics_primary_surface_height()
                                          , 0, 0
                                          , middleware_api_graphics_primary_surface_width()
                                          , middleware_api_graphics_primary_surface_height());
  dirty = false;
  condition_variable->notify_one();
}

}

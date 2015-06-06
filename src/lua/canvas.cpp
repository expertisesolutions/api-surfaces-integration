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

#include <middleware-api/graphics.h>

#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/move.hpp>
#include <middleware-integration/log.hpp>
#include <middleware-integration/graphics.hpp>

#include <lua.hpp>

#include <luabind/luabind.hpp>
#include <luabind/out_value_policy.hpp>

#include <boost/thread/mutex.hpp>

#include <iostream>

namespace middleware_integration { namespace ncl {

namespace lua { namespace {

void release_surface(middleware_api_graphics_surface_t* s)
{
  middleware_api_graphics_release_surface(*s);
}

struct canvas
{
  struct canvas_lock
  {
    canvas_lock(canvas const& d, canvas const& r)
    {
      boost::mutex* m = d.surface_mutex?d.surface_mutex
        :r.surface_mutex;
      if(m)
        lock.reset(new boost::unique_lock<boost::mutex>(*m));
    }
    canvas_lock(canvas const& c)
    {
      if(c.surface_mutex)
        lock.reset(new boost::unique_lock<boost::mutex>(*c.surface_mutex));
    }

    std::auto_ptr<boost::unique_lock<boost::mutex> > lock;
  };

  canvas(middleware_api_graphics_surface_t surface, dsmcc::dsmcc_filter& filter
         , middleware_integration::graphics& graphics
         , middleware_integration::window w
         , boost::mutex* surface_mutex = 0)
    :
#ifdef MIDDLEWARE_INTEGRATION_LUABIND_RVALUE_SUPPORT
    surface_(surface)
#else
    surface_(new middleware_api_graphics_surface_t(surface)
            , &lua::release_surface)
#endif
    , filter(&filter)
    , surface_mutex(surface_mutex)
    , graphics(graphics), window(window)
  {}

#ifdef MIDDLEWARE_INTEGRATION_LUABIND_RVALUE_SUPPORT
  canvas(BOOST_RV_REF(canvas) other)
    : surface_(other.surface_), filter(other.filter)
    , surface_mutex(other.surface_mutex)
    , graphics(other.graphics)
    , window(other.window)
  {
    assert(&graphics == &other.graphics);
    assert(window == other.window);
  }
  canvas& operator=(BOOST_RV_REF(canvas) other)
  {
    assert(&graphics == &other.graphics);
    assert(window == other.window);
    surface_ = other.surface_;
    filter = other.filter;
    surface_mutex = other.other.surface_mutex;
    return *this;
  }
#else
  canvas(canvas const& other)
    : surface_(other.surface_), filter(other.filter)
    , surface_mutex(other.surface_mutex)
    , graphics(other.graphics)
    , window(other.window)
  {}
#endif

  ~canvas()
  {
#ifdef MIDDLEWARE_INTEGRATION_LUABIND_RVALUE_SUPPORT
    middleware_api_graphics_release_surface(surface_);
#endif
  }

  canvas canvas_image_new(lua_State* L, std::string image_path)
  {
    MIDDLEWARE_INTEGRATION_DEBUG_LOG("canvas.new " << image_path.c_str());
    
    path root_path = luabind::object_cast<path>(luabind::registry(L)["root_path"]);
    dsmcc::dsmcc_filter* filter = luabind::object_cast<dsmcc::dsmcc_filter*>
      (luabind::registry(L)["filter"]);
    path p = root_path / path(image_path);
    std::cout << "Should search image in " << p.string() << std::endl;
    dsmcc::file_message const* file = dsmcc::find_file(*filter, p);
    if(file)
    {
      std::cout << "Found image" << std::endl;
      dsmcc::content_lock content (*filter, *file);
      middleware_api_graphics_surface_t s
        = middleware_api_graphics_load_image
        (content.buffer, content.size, ::middleware_api_graphics_image_format_png);
      return canvas(s, *filter, graphics, window);
    }
    else
      throw std::exception();
  }
  canvas canvas_buffer_new(int w, int h)
  {
    middleware_api_graphics_surface_t s
      = middleware_api_graphics_create_surface(w, h);
    return canvas(s, *filter, graphics, window);
  }
  void attrSize(int& width, int& height)
  {
    canvas_lock l(*this);
    width = middleware_api_graphics_width(surface());
    height = middleware_api_graphics_height(surface());
  }
  void attrScale(int w, int h)
  {
  }
  void get_attrScale(int& w, int& h)
  {
    w = 1; h = 1;
  }
  void compose(int x, int y, canvas const& src)
  {
    std::cout << "canvas::compose" << std::endl;
    canvas_lock l(*this, src);
    assert(src.surface() != 0);
    assert(surface() != 0);
    int sw = ::middleware_api_graphics_width(src.surface())
      , sh = ::middleware_api_graphics_height(src.surface());

    ::middleware_api_graphics_stretch_bitblit
        (surface(), src.surface(), x, y, sw, sh
         , 0u, 0u, sw, sh);
  }
  void compose_xy(int x, int y, canvas const& src, int sx, int sy
                  , int sw, int sh)
  {
    std::cout << "canvas::compose " << std::endl;
    canvas_lock l(*this, src);
    assert(src.surface() != 0);
    assert(surface() != 0);

    ::middleware_api_graphics_stretch_bitblit
        (surface(), src.surface(), x, y, sw, sh
         , sx, sy, sw, sh);
  }
  void flush()
  {
    if(surface_mutex)
      graphics.mark_dirty_and_wait_redraw(window);
  }
  void attrColor(int red, int green, int blue, int alpha)
  {
  }
  void attrColor_string(std::string color)
  {
  }
  void attrClip(int x, int y, int w, int h)
  {
    // clip_x = x; clip_y = y; clip_w = w; clip_h = h;
  }
  void get_attrClip(int& x, int& y, int& w, int& h)
  {
    // x = clip_x; y = clip_y; w = clip_w; h = clip_h;
  }
  void attrCrop (int x, int y, int w, int h)
  {
    // crop_x = x; crop_y = y; crop_w = w; crop_h = h;
  }
  void get_attrCrop (int& x, int& y, int& w, int& h)
  {
    // x = crop_x; y = crop_y; w = crop_w; h = crop_h;
  }
  void attrFlip(bool h, bool v)
  {
    // TODO:
  }
  void get_attrFlip(bool& h, bool& v)
  {
    // TODO:
  }
  void attrOpacity(int alpha)
  {
    // TODO:
  }
  int get_attrOpacity()
  {
    // TODO:
    return 0;
  }
  void attrRotation(int r)
  {
    // TODO:
  }
  int get_attrRotation()
  {
    // TODO:
    return 0;
  }
  void clear(int x, int y, int w, int h)
  {
  }

#ifdef MIDDLEWARE_INTEGRATION_LUABIND_RVALUE_SUPPORT
  middleware_api_graphics_surface_t surface() const { return surface_; }

  middleware_api_graphics_surface_t surface_;
#else
  middleware_api_graphics_surface_t surface() const { return *surface_; }

  boost::shared_ptr<middleware_api_graphics_surface_t> surface_;
#endif
  dsmcc::dsmcc_filter* filter;
  boost::mutex* surface_mutex;
  middleware_integration::graphics& graphics;
  middleware_integration::window window;

#ifdef MIDDLEWARE_INTEGRATION_LUABIND_RVALUE_SUPPORT
  BOOST_MOVABLE_BUT_NOT_COPYABLE(canvas)
#endif
};

} }

void init_canvas(lua_State* L, middleware_api_graphics_surface_t surface
                 , boost::mutex& mutex, middleware_integration::graphics& graphics
                 , middleware_integration::window window
                 , dsmcc::dsmcc_filter& filter)
{
  luabind::module(L, "ghtv")
  [
   luabind::class_<lua::canvas>("canvas")
   .def("new"         , &lua::canvas::canvas_image_new)
   .def("new"         , &lua::canvas::canvas_buffer_new)
   .def("attrSize"    , &lua::canvas::attrSize, luabind::pure_out_value(_2)
        | luabind::pure_out_value(_3))
   .def("attrScale"   , &lua::canvas::attrScale)
   .def("attrScale"   , &lua::canvas::get_attrScale, luabind::pure_out_value(_2)
        | luabind::pure_out_value(_3))
   .def("compose"     , &lua::canvas::compose)
   .def("compose"     , &lua::canvas::compose_xy)
   .def("flush"       , &lua::canvas::flush)
   .def("attrColor"   , &lua::canvas::attrColor)
   .def("attrColor"   , &lua::canvas::attrColor_string)
   .def("attrClip"    , &lua::canvas::attrClip)
   .def("attrClip"    , &lua::canvas::get_attrClip, luabind::pure_out_value(_2)
        | luabind::pure_out_value(_3) | luabind::pure_out_value(_4)
        | luabind::pure_out_value(_5))
   .def("attrCrop"    , &lua::canvas::attrCrop)
   .def("attrCrop"    , &lua::canvas::get_attrCrop, luabind::pure_out_value(_2)
        | luabind::pure_out_value(_3) | luabind::pure_out_value(_4)
        | luabind::pure_out_value(_5))
   .def("attrFlip"    , &lua::canvas::attrFlip)
   .def("attrFlip"    , &lua::canvas::get_attrFlip, luabind::pure_out_value(_2)
        | luabind::pure_out_value(_3))
   .def("attrOpacity" , &lua::canvas::attrOpacity)
   .def("attrOpacity" , &lua::canvas::get_attrOpacity)
   .def("attrRotation", &lua::canvas::attrRotation)
   .def("attrRotation", &lua::canvas::get_attrRotation)
   .def("attrScale"   , &lua::canvas::attrScale)
   .def("attrScale"   , &lua::canvas::get_attrScale, luabind::pure_out_value(_2)
        | luabind::pure_out_value(_3))
   .def("clear"       , &lua::canvas::clear)
  ];

  luabind::globals(L)["canvas"] = lua::canvas(surface, filter, graphics, window, &mutex);
}

} }


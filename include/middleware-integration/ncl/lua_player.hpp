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

#ifndef MIDDLEWARE_INTEGRATION_NCL_LUA_PLAYER_HPP
#define MIDDLEWARE_INTEGRATION_NCL_LUA_PLAYER_HPP

#include <middleware-integration/ncl/player.hpp>
#include <middleware-integration/graphics.hpp>
#include <middleware-integration/dsmcc.hpp>
#include <middleware-api/graphics.h>

#include <boost/thread.hpp>

#include <lua.hpp>

namespace middleware_integration { namespace ncl {

struct lua_reader_state
{
  const char* buffer;
  size_t size;
};

struct lua_player : player
{
  lua_player(dsmcc::dsmcc_filter& dsmcc_filter
             , dsmcc::file_message const& file
             , path lua_path, middleware_integration::graphics& graphics
             , middleware_integration::window w
             , middleware_api_graphics_surface_t surface
             , std::auto_ptr<boost::mutex> surface_mutex);

  void run();

  dsmcc::dsmcc_filter& dsmcc_filter;
  lua_State* lua_state, *lua_main_thread;
  boost::thread thread;
  std::auto_ptr<boost::mutex> surface_mutex;
};

} }

#endif

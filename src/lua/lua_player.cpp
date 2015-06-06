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

#include <middleware-integration/ncl/lua_player.hpp>
#include <middleware-integration/log.hpp>

#include <middleware-api/graphics.h>

#include <boost/thread.hpp>

#include <lua.hpp>
extern "C" {
#include <lualib.h>
}

#include <luabind/luabind.hpp>

#include <iostream>

namespace middleware_integration { namespace ncl {

void init_canvas(lua_State* L, middleware_api_graphics_surface_t surface
                 , boost::mutex& mutex, middleware_integration::graphics& graphics
                 , middleware_integration::window w
                 , dsmcc::dsmcc_filter& filter);
void init_event(lua_State* L);
void init_io(lua_State* L, dsmcc::dsmcc_filter& filter);
void init_global(lua_State* L, dsmcc::dsmcc_filter& filter
                 , path lua_path)
{
  luabind::module(L, "ghtv")
  [
     luabind::class_<path>("path")
   , luabind::class_<dsmcc::dsmcc_filter>("filter")
  ];
  luabind::registry(L)["root_path"] = lua_path.parent();
  luabind::registry(L)["filter"] = &filter;
}

namespace lua {

const char* lua_reader(lua_State*, void* data, size_t* size);

}

namespace {

void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
  (void)ud; (void)osize;
  if (nsize == 0)
  {
    free(ptr);
    return 0;
  }
  else
    return realloc(ptr, nsize);
}

}


lua_player::lua_player(dsmcc::dsmcc_filter& dsmcc_filter
                       , dsmcc::file_message const& file
                       , path lua_path, middleware_integration::graphics& graphics
                       , middleware_integration::window w
                       , middleware_api_graphics_surface_t surface
                       , std::auto_ptr<boost::mutex> surface_mutex_a)
  : dsmcc_filter(dsmcc_filter), lua_state(0)
  , lua_main_thread(0), surface_mutex(surface_mutex_a)
{
  MIDDLEWARE_INTEGRATION_DEBUG_LOG("lua_player::lua_player BEGIN");
  lua_state = lua_newstate(&lua_alloc, 0);
  if(lua_state)
  {
    luabind::open(lua_state);

    static const luaL_Reg lualibs[] =
      {
          {""             , & ::luaopen_base}
        , {LUA_LOADLIBNAME, & ::luaopen_package}
        , {LUA_TABLIBNAME , & ::luaopen_table}
        , {LUA_STRLIBNAME , & ::luaopen_string}
        , {LUA_MATHLIBNAME, & ::luaopen_math}
        , {LUA_DBLIBNAME  , & ::luaopen_debug}
        , {LUA_OSLIBNAME  , & ::luaopen_os}
      };

    for (std::size_t i = 0; i != sizeof(lualibs)/sizeof(lualibs[0]);++i)
    {
      lua_pushcfunction(lua_state, lualibs[i].func);
      lua_pushstring(lua_state, lualibs[i].name);
      lua_call(lua_state, 1, 0);
    }

    init_global(lua_state, dsmcc_filter, lua_path);
    init_event(lua_state);
    init_canvas(lua_state, surface, *surface_mutex, graphics, w, dsmcc_filter);
    init_io(lua_state, dsmcc_filter);

    luabind::object persistent_table = luabind::newtable(lua_state);
    persistent_table["service"] = luabind::newtable(lua_state);
    persistent_table["channel"] = luabind::newtable(lua_state);
    persistent_table["shared"] = luabind::newtable(lua_state);
    
    luabind::globals(lua_state)["persistent"] = persistent_table;

    MIDDLEWARE_INTEGRATION_DEBUG_LOG("LUAPLAYER persistent table created");

    lua_main_thread = lua_newthread(lua_state);
    dsmcc::content_lock content(dsmcc_filter, file);

    std::cout << "|" << boost::make_iterator_range
      (content.buffer, content.buffer + content.size)
              << "|" << std::endl;
    
    lua_reader_state state = {content.buffer, content.size};
    int r = lua_load(lua_main_thread, &lua::lua_reader, &state, "main.lua"
#ifdef MIDDLEWARE_INTEGRATION_LUA_52
                     , 0
#endif
                     );
    if(!r)
    {
      thread = boost::thread(&lua_player::run, this);
    }
    else
    {
      MIDDLEWARE_INTEGRATION_ERROR_LOG("Error loading lua " << r);
      if(lua_gettop(lua_main_thread) > 0)
      {
        luabind::object error_msg(luabind::from_stack(lua_main_thread, -1));
        if(luabind::type(error_msg) == LUA_TSTRING)
          MIDDLEWARE_INTEGRATION_ERROR_LOG
            ("Error " << luabind::object_cast<std::string>(error_msg));
      }
      else
        MIDDLEWARE_INTEGRATION_ERROR_LOG("No error message");
      throw std::exception();
    }
  }
  else
    throw std::exception();
  MIDDLEWARE_INTEGRATION_DEBUG_LOG("lua_player::lua_player END");
}

void lua_player::run()
{
  MIDDLEWARE_INTEGRATION_DEBUG_LOG("lua_player::run");
  int r = lua_resume(lua_main_thread, 0
#ifdef MIDDLEWARE_INTEGRATION_LUA_52
                     , 0
#endif
                     );
  if(r != 0)
  {
    if(r == LUA_YIELD)
      MIDDLEWARE_INTEGRATION_ERROR_LOG("Yielded");
    else
    {
      MIDDLEWARE_INTEGRATION_ERROR_LOG("Error running lua " << r);
      if(lua_gettop(lua_main_thread) > 0)
      {
        luabind::object error_msg(luabind::from_stack(lua_main_thread, -1));
        if(luabind::type(error_msg) == LUA_TSTRING)
          MIDDLEWARE_INTEGRATION_ERROR_LOG
            ("Error " << luabind::object_cast<std::string>(error_msg));
      }
      else
        MIDDLEWARE_INTEGRATION_ERROR_LOG("No error message");
    }
  }

}

} }

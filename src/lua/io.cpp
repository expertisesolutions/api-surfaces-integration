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

#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/ncl/lua_player.hpp>

#include <lua.hpp>

#include <luabind/luabind.hpp>

namespace middleware_integration { namespace ncl {

namespace lua {

const char* lua_reader(lua_State*, void* data, size_t* size)
{
  lua_reader_state* state = static_cast<lua_reader_state*>(data);
  *size = state->size;
  state->size = 0;
  return state->buffer;
}

namespace {

int loadfile(lua_State* L)
{
  if(lua_gettop(L) == 1)
  {
    luabind::object filename_obj (luabind::from_stack(L, 1));
    if(luabind::type(filename_obj) == LUA_TSTRING)
    {
      std::string filename = luabind::object_cast<std::string>(filename_obj);
      std::cout << "loadfile filename: " << filename << std::endl;
      path p = luabind::object_cast<path>(luabind::registry(L)["root_path"]);
      dsmcc::dsmcc_filter* filter = luabind::object_cast<dsmcc::dsmcc_filter*>
        (luabind::registry(L)["filter"]);
      std::cout << "root path " << p.string() << std::endl;
      path file_path = p / path(filename);
      std::cout << "Should search for " << file_path.string() << std::endl;
      dsmcc::file_message const* file = dsmcc::find_file(*filter, file_path);
      if(file)
      {
        std::cout << "File found" << std::endl;
        dsmcc::content_lock content(*filter, *file);
        std::cout << "|" << boost::make_iterator_range
          (content.buffer, content.buffer + content.size)
                  << "|" << std::endl;
        lua_reader_state state = {content.buffer, content.size};
        if(lua_load(L, &lua::lua_reader, &state, "main.lua"
#ifdef MIDDLEWARE_INTEGRATION_LUA_52
                    , 0
#endif
                    ) == 0)
          return 1;
      }
      else
      {
        std::cout << "File NOT found" << std::endl;
      }
    }
  }
  return 0;
}

const char io_lua_functions[] =
"function dofile(filename)\n"
" local f = assert(loadfile(filename))\n"
" return f()\n"
"end\n"
;

} }

void init_io(lua_State* L, dsmcc::dsmcc_filter& filter)
{
  lua_pushcfunction(L, &lua::loadfile);
  luabind::object loadfile_obj(luabind::from_stack(L, -1));
  lua_pop(L, 1);

  luabind::globals(L)["loadfile"] = loadfile_obj;

  int r = luaL_loadstring(L, lua::io_lua_functions);
  assert(r == 0);
  lua_call(L, 0, 0);
}

} }


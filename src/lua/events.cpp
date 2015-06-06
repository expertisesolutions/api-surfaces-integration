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

#include <middleware-integration/log.hpp>

#include <lua.hpp>

#include <luabind/luabind.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/tag_function.hpp>

namespace middleware_integration { namespace ncl {

namespace lua { namespace {

bool post_out(luabind::object event, std::string& error_msg)
{
  return false;
}

int register_(lua_State* L)
{
  return 0;
}

int uptime()
{
  return 0;
}

luabind::object timer(int time, luabind::object f)
{
  MIDDLEWARE_INTEGRATION_DEBUG_LOG("event.timer");
  return luabind::object();
}

} }

void init_event(lua_State* L)
{
  using luabind::tag_function;

  luabind::module(L, "event")
  [
   luabind::def("post", tag_function<bool(luabind::object, std::string&)>
                (boost::bind(&lua::post_out, _1, _2))
                , luabind::pure_out_value(_2))
   // , luabind::def("post", tag_function<bool(std::string, luabind::object, std::string&)>
   //                (boost::bind(&event::post, _1, _2, _3, this))
   //                , luabind::pure_out_value(_3))
   , luabind::def("timer", tag_function<luabind::object(int, luabind::object)>
                  (boost::bind(&lua::timer, _1, _2)))
   , luabind::def("uptime", &lua::uptime)
  ];

  lua_pushcfunction(L, &lua::register_);
  luabind::object register_(luabind::from_stack(L, -1));
  luabind::globals(L)["event"]["register"] = register_;
}

} }

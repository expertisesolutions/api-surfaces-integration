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

#ifndef MIDDLEWARE_INTEGRATION_MOVE_HPP
#define MIDDLEWARE_INTEGRATION_MOVE_HPP

#include <boost/config.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION >= 104900
#define MIDDLEWARE_INTEGRATION_MOVE_SUPPORT

#ifdef BOOST_HAS_RVALUE_REFS
#define MIDDLEWARE_INTEGRATION_LUABIND_RVALUE_SUPPORT
#endif

#include <boost/move/move.hpp>
#else
#define BOOST_COPY_ASSIGN_REF(x) x const&
#define BOOST_COPYABLE_AND_MOVABLE(x)
#define BOOST_MOVABLE_BUT_NOT_COPYABLE(x)
#endif

namespace middleware_integration {

template <typename T>
inline T& move(T& obj) { return obj; }

}

#endif

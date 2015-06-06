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
#include <middleware-integration/ncl/null_player.hpp>
#include <middleware-integration/ncl/player.hpp>
#include <middleware-integration/dsmcc.hpp>
#include <middleware-api/graphics.h>

#include <boost/shared_ptr.hpp>

namespace middleware_integration { namespace ncl {

player::~player()
{
}

boost::shared_ptr<player> create_player
 (gntl::parser::libxml2::dom::xml_string<> source
  , boost::optional<std::string> interface_
  , gntl::algorithm::structure::media::dimensions dim
  , gntl::structure::composed::component_location
  <std::string, std::string> location
  , descriptor_t descriptor
  , dsmcc::dsmcc_filter& dsmcc_filter
  , std::string const& base_directory
  , middleware_integration::graphics& graphics)
{
  std::cout << "base directory " << base_directory << std::endl;

  const char* source_first = 
    static_cast<const char*>(static_cast<const void*>(boost::begin(source)))
    , *source_last = static_cast<const char*>(static_cast<const void*>
                                              (boost::end(source)));
  path file_path = path(base_directory) / path(boost::make_iterator_range(source_first, source_last));
  dsmcc::file_message const* file
    = dsmcc::find_file(dsmcc_filter, file_path);
  boost::shared_ptr<player> p;
  try
  {
    if(file)
    {
      middleware_api_graphics_surface_t surface
        = middleware_api_graphics_create_surface(dim.width, dim.height);
      std::auto_ptr<boost::mutex> mutex(new boost::mutex);
      middleware_integration::window w
        = graphics.add_shared_surface(surface, dim.zindex, dim.x, dim.y, *mutex);
      p.reset(new lua_player(dsmcc_filter, *file, file_path, graphics
                             , w, surface, mutex));
    }
    else
      p.reset(new null_player);
  }
  catch(std::bad_alloc const&)
  {
    throw;
  }
  catch(std::exception const&)
  {
    p.reset(new null_player);
  }
  return p;
}


} }


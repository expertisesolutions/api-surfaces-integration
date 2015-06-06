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

#ifndef MIDDLEWARE_INTEGRATION_NCL_PLAYER_HPP
#define MIDDLEWARE_INTEGRATION_NCL_PLAYER_HPP

#include <middleware-api/graphics.h>
#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/graphics.hpp>

#include <gntl/parser/libxml2/dom/document.hpp>
#include <gntl/structure/composed/component_location.hpp>
#include <gntl/structure/composed/descriptor.hpp>
#include <gntl/algorithm/structure/media/dimensions.hpp>

namespace middleware_integration { namespace ncl {

typedef std::string document_uri_t;

typedef gntl::structure::composed::descriptor
<gntl::parser::libxml2::dom::descriptor
 , document_uri_t> descriptor_t;

struct player
{
  virtual ~player();

};

boost::shared_ptr<player> create_player
 (gntl::parser::libxml2::dom::xml_string<> source
  , boost::optional<std::string> interface_
  , gntl::algorithm::structure::media::dimensions dim
  , gntl::structure::composed::component_location
  <std::string, std::string> location
  , descriptor_t descriptor
  , middleware_integration::dsmcc::dsmcc_filter& dsmcc_filter
  , std::string const& base_directory
  , middleware_integration::graphics& graphics);

} }

#endif

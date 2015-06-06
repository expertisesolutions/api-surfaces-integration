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

#include <middleware-integration/ncl.hpp>
#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/ncl/player.hpp>
#include <middleware-integration/log.hpp>

#include <gntl/parser/libxml2/dom/xml_document.hpp>
#include <gntl/algorithm/structure/media/dimensions.hpp>

#include <gntl/algorithm/structure/context/start.ipp>
#include <gntl/algorithm/structure/context/start_normal_action.hpp>
#include <gntl/algorithm/structure/context/start_action_traits.ipp>
#include <gntl/algorithm/structure/component/start.ipp>
#include <gntl/algorithm/structure/port/start.ipp>
#include <gntl/algorithm/structure/port/start_action_traits.ipp>
#include <gntl/algorithm/structure/media/start.ipp>
#include <gntl/algorithm/structure/media/start_action_traits.ipp>

#include <boost/tuple/tuple.hpp>

#include <libxml/parser.h>

#include <limits>

namespace gntl { namespace concept { namespace structure {

void executor_traits<middleware_integration::ncl::executor>
::add_border (executor& e, int border_width, color_type color)
{
}

void executor_traits<middleware_integration::ncl::executor>
::remove_border (executor& e)
{
}

void executor_traits<middleware_integration::ncl::executor>
::start(executor& e, gntl::parser::libxml2::dom::xml_string<> source
        , boost::optional<std::string> interface_
        , gntl::algorithm::structure::media::dimensions dim
        , gntl::structure::composed::component_location
        <std::string, std::string> location
        , middleware_integration::dsmcc::dsmcc_filter& dsmcc_filter
        , std::string const& base_directory
        , middleware_integration::graphics& graphics)
{
  std::cout << "start source: " << source << std::endl;

  e.player = middleware_integration::ncl::create_player
    (source, interface_, dim, location, e.descriptor, dsmcc_filter
     , base_directory, graphics);
}

void  executor_traits<middleware_integration::ncl::executor>::abort(executor& e)
{
}

void  executor_traits<middleware_integration::ncl::executor>::stop(executor& e)
{
}

void  executor_traits<middleware_integration::ncl::executor>::pause(executor& e)
{
}

void  executor_traits<middleware_integration::ncl::executor>::resume(executor& e)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_time_begin(executor& e, component_identifier i
                               , time_duration begin_time)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_time_end(executor& e, component_identifier i
                             , time_duration end_time)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_time_begin_end(executor& e, component_identifier i
                                   , time_duration begin_time, time_duration end_time)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_frame_begin(executor& e, component_identifier i, unsigned int frame)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_frame_end(executor& e, component_identifier i, unsigned int frame)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_frame_begin_end(executor& e, component_identifier i
                                    , unsigned int begin_frame, unsigned int end_frame)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_npt_begin(executor& e, component_identifier i, int npt)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_npt_end(executor& e, component_identifier i, int npt)
{
}

void  executor_traits<middleware_integration::ncl::executor>::area_npt_begin_end(executor& e, component_identifier i, int begin_npt
                                  , int end_npt)
{
}

void  executor_traits<middleware_integration::ncl::executor>::start_area(executor& e, component_identifier area)
{
}

bool  executor_traits<middleware_integration::ncl::executor>::start_set_property(executor& e, std::string property_name
                                  , std::string property_value)
{
  return false;
}

bool  executor_traits<middleware_integration::ncl::executor>::commit_set_property(executor& e, std::string property_name)
{
  return false;
}

bool  executor_traits<middleware_integration::ncl::executor>::wants_keys(executor const& e)
{
  return false;
}

void  executor_traits<middleware_integration::ncl::executor>::explicit_duration(executor& e, time_duration explicit_duration)
{
}

} } }

namespace middleware_integration { namespace ncl {

presentation_factory::result_type presentation_factory::operator()(descriptor_t d) const
{
  std::cout << "presentation_factory::operator()" << std::endl;
  return result_type(executor(d), d);
}

document create_document(dsmcc::dsmcc_filter& dsmcc_filter
                         , dsmcc::file_message const& ncl
                         , path filename
                         , middleware_api_graphics_surface* surface)
{
  dsmcc::content_lock content(dsmcc_filter, ncl);

  assert(content.buffer != 0);
  assert(content.size != 0);
  
  std::string string_filename = filename.string();

  ::xmlParserCtxtPtr parser_context = ::xmlNewParserCtxt();
  assert(!!parser_context);
  ::xmlDocPtr xmldoc = ::xmlCtxtReadMemory
      (parser_context
       , content.buffer
       , content.size
       , string_filename.c_str()
       , 0, 0);

  boost::shared_ptr<gntl::parser::libxml2::dom::xml_document>
    xmldocument(new gntl::parser::libxml2::dom::xml_document(xmldoc)); // gets ownership
  gntl::parser::libxml2::dom::document parser_document(xmldocument->root());
  structure_document_t structure_document(parser_document);
  document d = {xmldocument, structure_document, &dsmcc_filter
                , filename.parent().string(), surface, surface};
  return d;
}

void start_document(document& d)
{
  gntl::algorithm::structure::media::dimensions screen_dimensions
    = {0, 0, middleware_api_graphics_width(d.surface)
       , middleware_api_graphics_height(d.surface), 0};

  descriptor_t descriptor;
  try
  {
    gntl::algorithm::structure::context::start (gntl::ref(d.structure_document.body)
                                                , d.structure_document.body_location()
                                                , descriptor
                                                , gntl::ref(d.structure_document)
                                                , screen_dimensions
                                                , gntl::ref(*d.dsmcc_filter)
                                                , d.base_directory
                                                , gntl::ref(d.graphics));
    MIDDLEWARE_INTEGRATION_DEBUG_LOG("Body context was started");
    register_graphics_to_draw_on_primary_surface(d.graphics);
  }
  catch(std::exception const& e)
  {
    std::cout << "Thrown exception while starting body context " << e.what() << std::endl;
    std::cout << "Exception: " << typeid(e).name() << std::endl;
  }
}

} }

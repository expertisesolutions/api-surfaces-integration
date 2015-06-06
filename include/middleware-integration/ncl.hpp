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

#ifndef MIDDLEWARE_INTEGRATION_NCL_HPP
#define MIDDLEWARE_INTEGRATION_NCL_HPP

#ifdef BOOST_FUSION_INVOKE_MAX_ARITY
#if BOOST_FUSION_INVOKE_MAX_ARITY < 10
#error BOOST_FUSION_INVOKE_MAX_ARITY must be defined as 10
#endif
#else
#define BOOST_FUSION_INVOKE_MAX_ARITY 10
#define BOOST_FUSION_INVOKE_PROCEDURE_MAX_ARITY 10
#define BOOST_FUSION_INVOKE_FUNCTION_OBJECT_MAX_ARITY 10
#endif

#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/path.hpp>
#include <middleware-integration/ncl/player.hpp>
#include <middleware-integration/graphics.hpp>

#include <gntl/structure/composed/document.hpp>
#include <gntl/parser/libxml2/dom/document.hpp>
#include <gntl/parser/libxml2/dom/xml_document.hpp>
#include <gntl/concept/structure/executor.hpp>
#include <gntl/algorithm/structure/media.hpp>

#include <middleware-api/graphics.h>

#include <boost/optional.hpp>

#include <string>

namespace middleware_integration { namespace ncl {

typedef std::string document_uri_t;

typedef gntl::structure::composed::descriptor
<gntl::parser::libxml2::dom::descriptor
 , document_uri_t> descriptor_t;

struct executor
{
  descriptor_t descriptor;
  boost::shared_ptr<ncl::player> player;

  executor(descriptor_t descriptor) {}
};

} }

namespace gntl { namespace concept { namespace structure {

template <>
struct executor_traits<middleware_integration::ncl::executor>
{
  typedef boost::mpl::true_ is_executor;
  typedef middleware_integration::ncl::executor executor;
  typedef boost::posix_time::time_duration time_duration;
  typedef gntl::parser::libxml2::dom::color color_type;
  typedef gntl::parser::libxml2::dom::xml_string<> component_identifier;
  typedef boost::mpl::vector
  <void(executor, gntl::parser::libxml2::dom::xml_string<>
        , boost::optional<std::string>
        , gntl::algorithm::structure::media::dimensions
        , gntl::structure::composed::component_location<std::string, std::string>
        , middleware_integration::dsmcc::dsmcc_filter&
        , std::string, middleware_integration::graphics&)> start_function_overloads;
  typedef boost::mpl::vector
  <void(executor)> stop_function_overloads;
  typedef boost::mpl::vector
  <void(executor)> pause_function_overloads;
  typedef boost::mpl::vector
  <void(executor)> resume_function_overloads;
  typedef boost::mpl::vector
  <void(executor)> abort_function_overloads;

  static void add_border (executor& e, int border_width, color_type color);
  static void remove_border (executor& e);

  static void start(executor& e, gntl::parser::libxml2::dom::xml_string<> source
                    , boost::optional<std::string> interface_
                    , gntl::algorithm::structure::media::dimensions dim
                    , gntl::structure::composed::component_location<std::string, std::string> location
                    , middleware_integration::dsmcc::dsmcc_filter& dsmcc_filter
                    , std::string const& base_directory
                    , middleware_integration::graphics&);
  static void abort(executor& e);
  static void stop(executor& e);
  static void pause(executor& e);
  static void resume(executor& e);

  static void area_time_begin(executor& e, component_identifier i, time_duration begin_time);
  static void area_time_end(executor& e, component_identifier i, time_duration end_time);
  static void area_time_begin_end(executor& e, component_identifier i, time_duration begin_time
                                  , time_duration end_time);
  static void area_frame_begin(executor& e, component_identifier i, unsigned int frame);
  static void area_frame_end(executor& e, component_identifier i, unsigned int frame);
  static void area_frame_begin_end(executor& e, component_identifier i, unsigned int begin_frame
                                   , unsigned int end_frame);
  static void area_npt_begin(executor& e, component_identifier i, int npt);
  static void area_npt_end(executor& e, component_identifier i, int npt);
  static void area_npt_begin_end(executor& e, component_identifier i, int begin_npt, int end_npt);
  static void start_area(executor& e, component_identifier area);
  static bool start_set_property(executor& e, std::string property_name, std::string property_value);
  static bool commit_set_property(executor& e, std::string property_name);
  static bool wants_keys(executor const& e);
  static void explicit_duration(executor& e, time_duration explicit_duration);
};

} } }

namespace middleware_integration { namespace ncl {

typedef gntl::structure::composed::presentation
<executor, descriptor_t> presentation_t;

struct presentation_factory
{
  typedef presentation_t result_type;
  result_type operator()(descriptor_t d) const;
};

typedef gntl::structure::composed::document
<gntl::parser::libxml2::dom::document
 , presentation_factory
 , document_uri_t> structure_document_t;

struct document
{
  boost::shared_ptr<gntl::parser::libxml2::dom::xml_document> xml_document;
  structure_document_t structure_document;
  dsmcc::dsmcc_filter* dsmcc_filter;
  std::string base_directory;
  middleware_api_graphics_surface* surface;
  middleware_integration::graphics graphics;
};

document create_document(dsmcc::dsmcc_filter& dsmcc_filter
                         , dsmcc::file_message const& ncl
                         , path filename, middleware_api_graphics_surface*);

void start_document(document& d);

} }

#endif

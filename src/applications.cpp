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

#include <middleware-integration/applications.hpp>
#include <middleware-integration/sections.hpp>

#include <gts/constants/etsi/protocol_id.hpp>
#include <gts/descriptors/transport_protocol_descriptor.hpp>
#include <gts/descriptors/ginga_application_location_descriptor.hpp>

#include <boost/bind.hpp>

#include <iostream>
#include <map>

namespace middleware_integration { namespace applications {

namespace {

std::map<unsigned int, application> apps;

void application_download_finish(unsigned int application_id)
{
  std::cout << "application_download_finish" << std::endl;

  std::map<unsigned int, application>::iterator iterator
    = apps.find(application_id);
  if(iterator != apps.end())
  {
    std::cout << "Total allocated for DSMCC "
              << dsmcc::allocation_size << std::endl;

    typedef std::vector<char>::const_iterator iterator_t;

    typedef gts::descriptors::ginga_application_location_descriptor<iterator_t>
      ginga_application_location_descriptor_t;
    typedef ginga_application_location_descriptor_t::base_directory_iterator
      base_directory_iterator;
    typedef ginga_application_location_descriptor_t::initial_class_iterator
      initial_class_iterator;

    ginga_application_location_descriptor_t ginga_location
      (iterator->second.ginga_location_descriptor.begin()
       , iterator->second.ginga_location_descriptor.end());
    
    base_directory_iterator base_it = ++ginga_location.begin();
    base_directory_iterator::deref_type base = *base_it;
    initial_class_iterator initial_class_it = ++ ++base_it;
    initial_class_iterator::deref_type initial_class = *initial_class_it;

    std::cout << "base directory  " << base << std::endl;
    std::cout << "Should open NCL at "
              << base << '/' << initial_class << std::endl;
    path ncl_path
      = path(boost::make_iterator_range
             (&*boost::begin(base)
              , &*boost::begin(base) + boost::distance(base)))
      / path(boost::make_iterator_range
             (&*boost::begin(initial_class)
              , &*boost::begin(initial_class) + boost::distance(initial_class)));
    dsmcc::file_message const* file = dsmcc::find_file
      (iterator->second.dsmcc, ncl_path);
    if(file)
    {
      iterator->second.ncl_document = ncl::create_document
        (iterator->second.dsmcc, *file, ncl_path
         , middleware_api_graphics_create_surface
         (middleware_api_graphics_primary_surface_width()
          , middleware_api_graphics_primary_surface_height()));
      ncl::start_document(iterator->second.ncl_document);
    }
    else
    {
      std::cout << "Couldn't find root ncl" << std::endl;
    }
  }
}

}

application::application(bool ncl, int application_id)
  : ncl(ncl), application_id(application_id)
  , control_code(gts::constants::abnt::application_control_codes::present)
{
}

void add_application(application app)
{
  std::map<unsigned int, application>::iterator it
    = apps.insert(std::make_pair(app.application_id, application())).first;
  swap(app, it->second);

  if(!it->second.transport_protocol_descriptor.empty()
     && !it->second.ginga_location_descriptor.empty())
  {
    typedef std::vector<char>::const_iterator iterator;
    typedef gts::descriptors::transport_protocol_descriptor<iterator>
      transport_protocol_descriptor;
    typedef transport_protocol_descriptor::protocol_id_iterator
      protocol_id_iterator;
    typedef transport_protocol_descriptor::transport_protocol_label_iterator
      transport_protocol_label_iterator;
    typedef transport_protocol_descriptor::selector_byte_iterator
      selector_byte_iterator;

    transport_protocol_descriptor transport_protocol
      (it->second.transport_protocol_descriptor.begin()
       , it->second.transport_protocol_descriptor.end());
    if(* ++transport_protocol.begin() == gts::constants::etsi::protocol_ids::object_carousel)
    {
      selector_byte_iterator selector_byte_it = gts::iterators::next<3>(transport_protocol.begin());
      typedef transport_protocol_descriptor::object_carousel_transport object_carousel_transport;
      object_carousel_transport transport(*selector_byte_it);
      typedef object_carousel_transport::component_tag_iterator component_tag_iterator;
      component_tag_iterator component_it = gts::iterators::next<4>(transport.begin());
      
      std::cout << "=== Should filter component " << *component_it << std::endl;
      boost::optional<unsigned int> pid = sections::pid_from_component_tag(*component_it);
      if(pid)
      {
        dsmcc::start(it->second.dsmcc, *pid
                     , boost::bind(&application_download_finish
                                   , it->second.application_id));
      }
      else
        std::cout << "Couldn't find ES for component tag" << std::endl;
    }
    else
    {
      std::cout << "protocol id is " << * ++transport_protocol.begin() << std::endl;
    }
  }
  else
  {
    std::cout << "transport_protocol is empty" << std::endl;
  }
}

} }

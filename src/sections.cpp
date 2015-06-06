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

#include <middleware-integration/move.hpp>
#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/applications.hpp>
#include <middleware-integration/sections.hpp>

#include <middleware-api/sections.h>
#include <middleware-api/applications.h>

#include <gts/constants/pmt/stream_type.hpp>
#include <gts/constants/arib/descriptor_tag.hpp>
#include <gts/constants/etsi/descriptor_tag.hpp>
#include <gts/constants/etsi/table_id.hpp>
#include <gts/constants/abnt/data_component_id.hpp>
#include <gts/constants/abnt/application_type.hpp>
#include <gts/constants/abnt/application_control_code.hpp>
#include <gts/descriptors/data_component_descriptor.hpp>
#include <gts/descriptors/stream_identifier_descriptor.hpp>
#include <gts/sections/program_association_section.hpp>
#include <gts/sections/program_map_section.hpp>
#include <gts/sections/application_information_section.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <cassert>

#include <iostream>
#include <vector>

namespace middleware_integration { namespace sections {

namespace {

struct elementary_stream
{
  unsigned int pid;
  unsigned int component_tag;
  unsigned int stream_type;
};

inline bool operator<(elementary_stream const& lhs, elementary_stream const& rhs)
{
  return lhs.pid < rhs.pid;
}

typedef boost::multi_index::multi_index_container
<elementary_stream
 , boost::multi_index::indexed_by
   <
     boost::multi_index::ordered_unique
      <boost::multi_index::identity<elementary_stream> >
   , boost::multi_index::ordered_non_unique
      <boost::multi_index::member
       <elementary_stream, unsigned int, &elementary_stream::component_tag>
      >
   >
> elementary_stream_container;

elementary_stream_container elementary_streams;

void read_ait(const char* buffer, size_t size
              , middleware_api_sections_filter* ait_filter, void*)
{
  // std::cout << "Read AIT" << std::endl;
  middleware_api_sections_remove_filter(ait_filter);

  typedef const char* iterator;
  typedef gts::sections::application_information_section<iterator>
    application_information_section;
  typedef application_information_section::application_type_iterator application_type_iterator;
  typedef application_information_section::current_next_indicator_iterator current_next_indicator_iterator;
  typedef application_information_section::descriptors_iterator descriptors_iterator;
  typedef application_information_section::applications_iterator applications_iterator;

  application_information_section ait(buffer, buffer + size);
  
  application_type_iterator application_type_it = gts::iterators::next<3>(ait.begin());
  //if(*application_type_it == gts::constants::abnt::application_types::ginga_ncl)
  {
    descriptors_iterator descriptors_it = gts::iterators::next<5>(application_type_it);
    typedef descriptors_iterator::deref_type descriptor_range;
    descriptor_range descriptors = *descriptors_it;
    typedef boost::range_iterator<descriptor_range>::type descriptor_iterator;
    for(descriptor_iterator first = boost::begin(descriptors), last = boost::end(descriptors)
          ;first != last; ++first)
    {
      // std::cout << "common AIT descriptor tag: " << *first->begin() << std::endl;
    }

    applications_iterator applications_it = gts::iterators::next(descriptors_it);
    typedef applications_iterator::deref_type application_range;
    application_range applications = *applications_it;
    typedef boost::range_iterator<application_range>::type application_iterator;
    for(application_iterator first = boost::begin(applications), last = boost::end(applications)
          ;first != last; ++first)
    {
      applications::application app(true, * ++ first->begin());

      // std::cout << "application" << std::endl;
      typedef boost::range_value<application_range>::type application_type;
      typedef application_type::application_control_code_iterator application_control_code_iterator;
      typedef application_type::application_descriptors_iterator application_descriptors_iterator;
      // std::cout << "application identifier: " << *++ first->begin() << std::endl;

      typedef application_descriptors_iterator::deref_type descriptor_range;
      descriptor_range descriptors = *++ ++ ++first->begin();
      typedef boost::range_iterator<descriptor_range>::type descriptor_iterator;
      for(descriptor_iterator first = boost::begin(descriptors)
            , last = boost::end(descriptors); first != last; ++first)
      {
        typedef boost::range_value<descriptor_range>::type descriptor_type;
        descriptor_type descriptor = *first;
        switch(*descriptor.begin())
        {
        case gts::constants::arib::descriptor_tags::application_descriptor:
          {
            app.application_descriptor.insert
              (app.application_descriptor.end()
               , descriptor.base_begin(), descriptor.base_end());
          }
          break;
        case gts::constants::arib::descriptor_tags::application_name_descriptor:
          {
            app.application_name_descriptor.insert
              (app.application_name_descriptor.end()
               , descriptor.base_begin(), descriptor.base_end());
          }
          break;
        case gts::constants::arib::descriptor_tags::transport_protocol_descriptor:
          {
            app.transport_protocol_descriptor.insert
              (app.transport_protocol_descriptor.end()
               , descriptor.base_begin(), descriptor.base_end());
          }
          break;
        case gts::constants::arib::descriptor_tags::ginga_j_application_descriptor:
          std::cout << "ginga_j_application_descriptor" << std::endl;
          break;
        case gts::constants::arib::descriptor_tags::ginga_j_application_location_descriptor:
          if(*application_type_it == gts::constants::abnt::application_types::ginga_j)
          {
            app.ginga_location_descriptor.insert
              (app.ginga_location_descriptor.end()
               , descriptor.base_begin(), descriptor.base_end());
          }
          break;
        case gts::constants::arib::descriptor_tags::ginga_ncl_application_location_descriptor:
          if(*application_type_it == gts::constants::abnt::application_types::ginga_ncl)
          {
            app.ginga_location_descriptor.insert
              (app.ginga_location_descriptor.end()
               , descriptor.base_begin(), descriptor.base_end());
          }
          break;
        default:
          ;
          // std::cout << "descriptor tag: " << *first->begin() << std::endl;
        }

      }

      app.control_code = * ++ ++ first->begin();
      applications::add_application(move(app));
    }
  }
}

}

void start_filtering(unsigned int service_id, const char* pat_buffer, std::size_t pat_size
                     , const char* pmt_buffer, std::size_t pmt_size)
{
  typedef const char* iterator;
  typedef gts::sections::program_map_section<iterator> program_map_section_type;
  typedef program_map_section_type::current_next_indicator_iterator current_next_indicator_iterator;
  typedef program_map_section_type::descriptor_repetition_iterator descriptor_repetition_iterator;
  typedef program_map_section_type::program_map_repetition_iterator program_map_repetition_iterator;
  typedef program_map_section_type::end_iterator end_iterator;

  program_map_section_type program_map_section(pmt_buffer, pmt_buffer + pmt_size);

  current_next_indicator_iterator current_next_indicator_it
    = gts::iterators::next<6>(program_map_section.begin());
  if(*current_next_indicator_it)
  {
    descriptor_repetition_iterator descriptor_repetition_it
      = gts::iterators::next<5>(current_next_indicator_it);
    program_map_repetition_iterator program_map_repetition_it
      = ++descriptor_repetition_it;
    typedef program_map_repetition_iterator::deref_type program_map_repetition_range;
    program_map_repetition_range program_map_repetition = *program_map_repetition_it;
    typedef boost::range_iterator<program_map_repetition_range>::type
      program_map_iterator;
    for(program_map_iterator first = boost::begin(program_map_repetition)
          , last = boost::end(program_map_repetition); first != last; ++first)
    {
      typedef boost::range_value<program_map_repetition_range>::type
        program_map_type;
      typedef program_map_type::stream_type_iterator stream_type_iterator;
      typedef program_map_type::elementary_pid_iterator elementary_pid_iterator;
      typedef program_map_type::es_info_length_iterator es_info_length_iterator;
      typedef program_map_type::descriptor_repetition_iterator descriptor_repetition_iterator;
      stream_type_iterator stream_type_it = first->begin();
      elementary_pid_iterator elementary_pid = gts::iterators::next(stream_type_it);
      unsigned int component_tag = 0;

      program_map_type program_map = *first;

      // std::cout << "stream_type: 0x" << std::hex << *program_map.begin() << std::dec << std::endl;
      // std::cout << "elementary_pid: " << std::hex << *++program_map.begin()
      //           << std::dec << std::endl;

      // for generic
      {
        descriptor_repetition_iterator descriptor_repetition_it
          = ++ ++ ++ program_map.begin();
        typedef descriptor_repetition_iterator::deref_type descriptor_range;
        typedef boost::range_iterator<descriptor_range>::type descriptor_iterator;
        descriptor_range descriptors = *descriptor_repetition_it;
        for(descriptor_iterator dfirst = boost::begin(descriptors)
              , dlast = boost::end(descriptors); dfirst != dlast; ++dfirst)
        {
          typedef boost::range_value<descriptor_range>::type descriptor_type;
          switch(*dfirst->begin())
          {
          default:
            // std::cout << "Tag for descriptor " << *dfirst->begin() << std::endl;
            break;
          case gts::constants::etsi::descriptor_tags::stream_identifier_descriptor:
            {
              // std::cout << "--- Stream identifier" << std::endl;
              typedef gts::descriptors::stream_identifier_descriptor<const char*>
                stream_identifier_descriptor_t;

              stream_identifier_descriptor_t stream_identifier_descriptor (*dfirst);
              component_tag = *++stream_identifier_descriptor.begin();
              // std::cout << "This stream has component tag: "
              //           << *++stream_identifier_descriptor.begin() << std::endl;
            }
            break;
          case gts::constants::arib::descriptor_tags::data_component_descriptor:
            {
              // std::cout << "--- Data component" << std::endl;

              typedef gts::descriptors::data_component_descriptor<const char*>
                data_component_descriptor_t;
              data_component_descriptor_t data_component_descriptor(*dfirst);

              if(*++data_component_descriptor.begin()
                 == gts::constants::abnt::data_component_ids::ait)
              {
                // std::cout << "data component with ait" << std::endl;
                if(*stream_type_it == gts::constants::pmt::stream_types::private_sections)
                {
                  // std::cout << "might contain AIT" << std::endl;
                  descriptor_repetition_iterator descriptors_it = gts::iterators::next<3>
                    (stream_type_it);
                  typedef descriptor_repetition_iterator::deref_type descriptor_range;
                  descriptor_range descriptors = *descriptors_it;
                  typedef boost::range_iterator<descriptor_range>::type descriptor_iterator;
                  for(descriptor_iterator first = boost::begin(descriptors)
                        , last = boost::end(descriptors); first != last; ++first)
                  {
                    typedef boost::range_value<descriptor_range>::type descriptor_type;
                    typedef descriptor_type::tag_iterator tag_iterator;
                    tag_iterator tag_it = first->begin();
                    if(*tag_it == gts::constants::arib::descriptor_tags::data_component_descriptor)
                    {
                      // std::cout << "Found ES with AIT" << std::endl;
                      middleware_api_sections_create_filter_for_pid_and_table_id
                        (*elementary_pid, gts::constants::etsi::table_ids::ait
                         , &read_ait, 0);
                    }
                  }
                }
              }
              else
              {
                // std::cout << "Data component is not for AIT "
                //           << *++data_component_descriptor.begin() << std::endl;
              }
            }
            break;
          }

          
        }
      }

      elementary_stream s = {*elementary_pid, component_tag, *stream_type_it};
      elementary_streams.insert(s);
    }
  }
}

void stop_filtering()
{
  elementary_streams.clear();
}

boost::optional<unsigned int> pid_from_component_tag(unsigned int tag)
{
  typedef elementary_stream_container::nth_index<1>::type index_type;
  index_type& index = elementary_streams.get<1>();
  typedef index_type::const_iterator iterator;
  iterator it = index.find(tag);
  if(it != index.end())
    return it->pid;
  else
    return boost::none;
} 

} }


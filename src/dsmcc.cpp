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
#include <middleware-integration/callback.hpp>
#include <middleware-integration/sections.hpp>

#include <gts/constants/mpeg/dsmcc_types.hpp>
#include <gts/dsmcc/sections/user_network_message_section.hpp>
#include <gts/dsmcc/descriptors/group_info_indication.hpp>
#include <gts/dsmcc/descriptors/service_gateway_info.hpp>
#include <gts/dsmcc/download_message_id.hpp>
#include <gts/dsmcc/download_server_initiate.hpp>
#include <gts/dsmcc/download_info_indication.hpp>
#include <gts/dsmcc/download_data_block.hpp>

#include <boost/bind.hpp>

#include <iostream>
#include <fstream>

namespace middleware_integration { namespace dsmcc {

unsigned int allocation_size = 0;

namespace {

void ddb_filtering(dsmcc_filter* self, unsigned int pid, const char* buffer
                   , std::size_t size, middleware_api_sections_filter* filter
                   , boost::function<void()> callback)
{
  typedef gts::dsmcc::sections::user_network_message_section
    <char const*> section_un_type;
  section_un_type dsmcc_section(buffer, buffer + size);
  typedef section_un_type::user_network_message_iterator
    user_network_message_iterator;
  user_network_message_iterator user_network_message_it
    = gts::iterators::next<9>(dsmcc_section.begin());
  typedef gts::dsmcc::download_data_block<char const*>
    download_data_block;
  typedef download_data_block::download_id_iterator download_id_iterator;
  typedef download_data_block::module_id_iterator module_id_iterator;
  typedef download_data_block::module_version_iterator module_version_iterator;
  typedef download_data_block::block_number_iterator block_number_iterator;
  typedef download_data_block::block_data_iterator block_data_iterator;
  download_data_block ddb(*user_network_message_it);
  download_id_iterator download_id_it = gts::iterators::next<3>(ddb.begin());
  if(*download_id_it == self->download_id)
  {
    module_id_iterator module_id_it = gts::iterators::next<2>(download_id_it);
    module_version_iterator module_version_it = ++module_id_it;
    block_number_iterator block_number_it = ++module_version_it;
    block_data_iterator block_data_it = ++block_number_it;

    // std::cout << "block module: " << *module_id_it << " block: "
    //           << *block_number_it << std::endl;

    std::map<unsigned int, downloading_module>::iterator
      iterator = self->modules_downloading.find(*module_id_it);
    if(iterator != self->modules_downloading.end())
    {
      // std::cout << "Module not downloaded yet "
      //           << std::count(iterator->second.blocks_downloaded.begin()
      //                         , iterator->second.blocks_downloaded.end(), 0)
      //           << std::endl;
      if(*module_version_it == iterator->second.version
         && !iterator->second.blocks_downloaded[*block_number_it])
      {
        typedef block_data_iterator::deref_type block_data_range;
        block_data_range block_data = *block_data_it;
        std::size_t offset = self->block_size* *block_number_it;

        iterator->second.data.reserve(iterator->second.size);

        if(iterator->second.data.size() >= offset + boost::distance(block_data))
        {
          std::copy(boost::begin(block_data), boost::end(block_data)
                    , &iterator->second.data[offset]);
        }
        else
        {
          if(iterator->second.data.size() < offset)
            iterator->second.data.resize(offset);
          iterator->second.data.insert(iterator->second.data.end()
                                       , boost::begin(block_data)
                                       , boost::end(block_data));
        }

        if(iterator->second.size != iterator->second.data.capacity())
        {
          // std::cout << "Capacity and size are different "
          //           << iterator->second.data.capacity()
          //           << " " << iterator->second.size
          //           << " " << iterator->second.data.size()
          //           << std::endl;
        }

        iterator->second.blocks_downloaded[*block_number_it] = true;

        if(iterator->second.finished())
        {
          {
            std::map<unsigned int, downloaded_module>::iterator
              downloaded_iterator = self->modules_downloaded.find(*module_id_it);
            if(downloaded_iterator != self->modules_downloaded.end())
            {
              typedef message_container::nth_index<0u>::type index_type;
              index_type& index = self->messages.get<0u>();
              std::pair<index_type::iterator, index_type::iterator>
                message_range = index.equal_range(*module_id_it);
              index.erase(message_range.first, message_range.second);
              self->modules_downloaded.erase(downloaded_iterator);
            }
          }

          // std::cout << "Module " << iterator->second.id << " has finished" << std::endl;
          boost::shared_ptr<buffer_container> data(new buffer_container);
          downloaded_module m = {iterator->second.id, iterator->second.version
                                 , data};
          std::swap(*m.data, iterator->second.data);
          std::map<unsigned int, downloaded_module>::iterator
            downloaded_iterator = self->modules_downloaded.insert
            (std::make_pair(iterator->second.id, m)).first;
          self->modules_downloading.erase(iterator);

          dsmcc_iterator first = data->begin(), last = data->end();
          {
            std::ofstream file("module.bin", std::ios::binary);
            std::copy(first, last, std::ostream_iterator<char>(file));
          }
          while(first != last)
          {
            typedef gts::dsmcc::biop::message_header
              <dsmcc_iterator>
              message_header_type;
            typedef message_header_type::magic_iterator magic_iterator;
            typedef message_header_type::biop_version_iterator biop_version_iterator;
            typedef message_header_type::byte_order_iterator byte_order_iterator;
            typedef message_header_type::message_type_iterator message_type_iterator;
            typedef message_header_type::message_size_iterator message_size_iterator;

            assert(first < last);

            message_header_type message_header(first, last);
            if(message_header.begin() == message_header.end())
              break;

            magic_iterator::deref_type magic = *message_header.begin();
            if(magic == 0x42494F50)
            {
              typedef gts::dsmcc::biop::message_subheader
                <dsmcc_iterator>
                message_subheader_type;
              typedef message_subheader_type::object_key_iterator object_key_iterator;
              typedef message_subheader_type::object_kind_iterator object_kind_iterator;
              typedef message_subheader_type::object_info_iterator object_info_iterator;
              typedef message_subheader_type::service_context_list_iterator service_context_list_iterator;
              typedef message_subheader_type::end_iterator subheader_end_iterator;
      
              message_size_iterator message_size_it = gts::iterators::next<4>(message_header.begin());
              std::size_t message_size = *message_size_it;
              // std::cout << "Message size: " << message_size << std::endl;

              dsmcc_iterator old_first = first;
              first = boost::next(first, (std::min<std::size_t>)
                                  (message_size
                                   + std::distance(first, (++message_size_it).base())
                                   , std::distance(first, last)));
              assert(first <= last);
              assert(first > old_first);

              dsmcc_iterator subheader_iterator
                = gts::iterators::next<5>(message_header.begin()).base();
              message_subheader_type message_subheader
                (subheader_iterator, first);
              object_key_iterator object_key_it = message_subheader.begin();
              object_kind_iterator object_kind_it = gts::iterators::next(object_key_it);
              object_info_iterator object_info_it = gts::iterators::next(object_kind_it);

              dsmcc::bind_info bind_info = {*object_key_it, *module_id_it};

              object_kind_iterator::deref_type kind = *object_kind_it;
              if((boost::distance(kind) == 4
                  && std::equal(kind.begin(), kind.end(), "srg"))
                 || (boost::distance(kind) == 4
                     && std::equal(kind.begin(), kind.end(), "dir")))
              {
                subheader_end_iterator message_subheader_end 
                  = gts::iterators::next<2>(object_info_it);

                typedef directory_message::message_body_length_iterator
                  message_body_length_iterator;
                typedef directory_message::binds_iterator binds_iterator;
                dsmcc::directory dir
                  = {directory_message(message_subheader_end.base(), first)};

                if(boost::distance(kind) == 4
                   && std::equal(kind.begin(), kind.end(), "srg"))
                  self->service_gateway_bind = bind_info;
                
                typedef binds_iterator::deref_type bind_range;
                bind_range binds = *++dir.message.begin();
                typedef boost::range_iterator<bind_range>::type bind_iterator;
                // std::cout << "binds " << boost::distance(binds) << std::endl;
                for(bind_iterator bfirst = boost::begin(binds), blast = boost::end(binds)
                      ;bfirst != blast; ++bfirst)
                {
                  // std::cout << "Bind" << std::endl;
                  assert(!bfirst->ior.biop_profiles.empty());
                  dsmcc::bind_info info = 
                    {bfirst->ior.biop_profiles[0].object_location.object_key
                     , bfirst->ior.biop_profiles[0].object_location.module_id};

                  dsmcc_iterator name_end
                    = boost::begin(bfirst->names[0].id);

                  while(name_end != boost::end(bfirst->names[0].id) && *name_end != 0)
                    ++name_end;

                  boost::iterator_range<dsmcc_iterator> name
                    (boost::begin(bfirst->names[0].id)
                     , name_end);

                  child_ref c = {name, info};
                  dir.childs.push_back(c);
                }
                message msg = {bind_info, dir};
                self->messages.get<0>().insert(msg);
              }
              else if(boost::distance(kind) == 4 && std::equal(kind.begin(), kind.end(), "fil"))
              {
                message msg = {bind_info, file_message(object_info_it.base(), first)};
                self->messages.get<0>().insert(msg);
              }
            }
          }
          
          // if(*module_id_it == self->service_gateway_module_id)
          //   std::cout << "Root module was downloaded" << std::endl;

          // std::cout << "Downloaded " << self->modules_downloaded.size()
          //           << "/"
          //           << self->modules_downloading.size() + self->modules_downloaded.size()
          //           << std::endl;

          // std::cout << "Still to download " << self->modules_downloading.size() << std::endl;
        }

        if(self->modules_downloading.empty())
        {
          // std::cout << "All modules were downloaded" << std::endl;
          //stop_filter_for_pid(*self, pid);
          assert(*self->ddb_filter = filter);
          self->ddb_filter = boost::none;
          middleware_api_sections_remove_filter(filter);

          callback();
        }
      }
      else
      {
        // if(*module_version_it != iterator->second.version)
        //   std::cout << "Wrong version " << *module_version_it
        //             << " expected " << iterator->second.version << std::endl;
        // else if(iterator->second.blocks_downloaded[*block_number_it])
        //   std::cout << "Has already been downloaded" << std::endl;
          
      }
    }
    // else
    //   std::cout << "Already downloaded module" << std::endl;
  }
}

void dii_filtering(dsmcc_filter* self, unsigned int pid, const char* buffer
                   , std::size_t size, middleware_api_sections_filter* filter
                   , boost::function<void()> callback);

void update_dii(dsmcc_filter* self, unsigned int pid
                , middleware_api_sections_filter* filter
                , gts::dsmcc::sections::user_network_message_section<char const*>
                ::user_network_message_iterator::deref_type const& un
                , boost::function<void()> callback)
{
  typedef gts::dsmcc::download_server_initiate<char const*>
    download_server_initiate_t;
  download_server_initiate_t dsi(un);
  typedef download_server_initiate_t::transaction_id_iterator transaction_id_iterator;
  typedef download_server_initiate_t::compatibility_descriptor_iterator
    compatibility_descriptor_iterator;
  typedef download_server_initiate_t::private_data_iterator private_data_iterator;

  transaction_id_iterator transaction_id_it = gts::iterators::next<3u>(dsi.begin());
  self->dsi_transaction_id = *transaction_id_it;

    // std::cout << "DSI transaction " << *transaction_id_it << std::endl;

    compatibility_descriptor_iterator compatibility_descriptor_it
      = gts::iterators::next<3>(transaction_id_it);
    compatibility_descriptor_iterator::deref_type compatibility_descriptor
      = *compatibility_descriptor_it;
    private_data_iterator::deref_type private_data = *++compatibility_descriptor_it;

    typedef gts::dsmcc::descriptors::service_gateway_info
      <char const*> service_gateway_info_t;
    service_gateway_info_t sgi(boost::begin(private_data)
                             , boost::end(private_data));
    typedef service_gateway_info_t::object_ref_iterator object_ref_iterator;

    object_ref_iterator::deref_type object_ref = *sgi.begin();

    if(object_ref.biop_profiles.size())
    {
      // std::cout << "Has biop profile" << std::endl;

      boost::optional<unsigned int> dii_pid
        = sections::pid_from_component_tag(object_ref.biop_profiles[0].conn_binder
                                           .delivery_para_use_tap.assoc_tag);
      if(dii_pid)
      {
        middleware_api_sections_remove_filter(filter);

        self->dii_transaction_id
          = object_ref.biop_profiles[0].conn_binder.delivery_para_use_tap.transaction_id;
        self->service_gateway_module_id = object_ref.biop_profiles[0].object_location
          .module_id;

        // std::cout << "Found PID for DII's " << *dii_pid << std::endl;
        self->waiting_dii = true;
        middleware_api_sections_create_filter_for_pid_and_table_id
          (*dii_pid, gts::constants::mpeg::dsmcc_types::user_network
           , &callbacks::section_callback
           , callbacks::section_callback_state
           (boost::bind(&dii_filtering, self, *dii_pid, _1, _2, _3, callback)));
      }
    }
}

void dsi_update_filtering(dsmcc_filter* self, unsigned int pid, const char* buffer
                          , std::size_t size, middleware_api_sections_filter* filter
                          , boost::function<void()> callback)
{
  // std::cout << "dsi_update_filtering" << std::endl;
  typedef gts::dsmcc::sections::user_network_message_section
    <char const*> section_un_type;
  section_un_type dsmcc_section(buffer, buffer + size);
  typedef section_un_type::user_network_message_iterator
    user_network_message_iterator;
  user_network_message_iterator user_network_message_it
    = gts::iterators::next<9>(dsmcc_section.begin());
  typedef user_network_message_iterator::deref_type user_network_message;
  user_network_message un = *user_network_message_it;
  typedef user_network_message::message_id_iterator message_id_iterator;
  message_id_iterator message_id_it = gts::iterators::next<2>(un.begin());
  if(*message_id_it == gts::dsmcc::download_message_id::download_server_initiate)
  {
    typedef gts::dsmcc::download_server_initiate<char const*>
      download_server_initiate_t;
    typedef download_server_initiate_t::transaction_id_iterator transaction_id_iterator;

    download_server_initiate_t dsi(un);
    
    transaction_id_iterator transaction_id_it = gts::iterators::next<3u>(dsi.begin());
    if(*transaction_id_it != self->dsi_transaction_id)
    {
      // std::cout << "DSI has changed transaction id " << *transaction_id_it << std::endl;

      update_dii(self, pid, filter, un, callback);
    }
  }
}

void dii_filtering(dsmcc_filter* self, unsigned int pid, const char* buffer
                   , std::size_t size, middleware_api_sections_filter* filter
                   , boost::function<void()> callback)
{
  // std::cout << "DII Filtering " << self->dii_transaction_id << std::endl;
  typedef gts::dsmcc::sections::user_network_message_section
    <char const*> section_un_type;
  section_un_type dsmcc_section(buffer, buffer + size);
  typedef section_un_type::user_network_message_iterator
    user_network_message_iterator;
  user_network_message_iterator user_network_message_it
    = gts::iterators::next<9>(dsmcc_section.begin());
  typedef user_network_message_iterator::deref_type user_network_message;
  user_network_message un = *user_network_message_it;
  typedef user_network_message::message_id_iterator message_id_iterator;
  message_id_iterator message_id_it = gts::iterators::next<2>(un.begin());
  // std::cout << "message_id: " << *message_id_it << std::endl;
  if(self->waiting_dii
     && *message_id_it == gts::dsmcc::download_message_id::download_info_indication)
  {
    // std::cout << "Waiting DII" << std::endl;
    typedef gts::dsmcc::download_info_indication
      <char const*> download_info_indication_t;
    download_info_indication_t dii(un);
    typedef download_info_indication_t::transaction_id_iterator transaction_id_iterator;
    typedef download_info_indication_t::download_id_iterator download_id_iterator;
    typedef download_info_indication_t::block_size_iterator block_size_iterator;
    typedef download_info_indication_t::module_range_iterator module_range_iterator;
    transaction_id_iterator transaction_id_it = gts::iterators::next<3>(dii.begin());
    if(*transaction_id_it == self->dii_transaction_id)
    {
      self->download_id = *gts::iterators::next<5>(dii.begin());
      self->block_size = *gts::iterators::next<6>(dii.begin());

      std::size_t modules_size = 0;
      module_range_iterator module_range_it = gts::iterators::next<12>(dii.begin());
      typedef module_range_iterator::deref_type module_range;
      module_range modules = *module_range_it;
      // std::cout << "There are " << boost::distance(modules) << std::endl;
      typedef boost::range_iterator<module_range>::type module_iterator;
      for(module_iterator first = boost::begin(modules)
            , last = boost::end(modules); first != last; ++first)
      {
        typedef boost::range_value<module_range>::type module_type;
        typedef module_type::module_id_iterator module_id_iterator;
        typedef module_type::module_size_iterator module_size_iterator;
        typedef module_type::module_version_iterator module_version_iterator;
        module_type module = *first;
        module_id_iterator module_id_it = module.begin();
        module_size_iterator module_size_it = ++module_id_it;
        module_version_iterator module_version_it = ++module_size_it;
        // std::cout << "module version: " << *module_version_it << std::endl;

        modules_size += *module_size_it;

        std::size_t number_of_blocks = *module_size_it/self->block_size;
        if(number_of_blocks*self->block_size < *module_size_it)
          ++number_of_blocks;

        downloading_module m = {*module_id_it, *module_size_it
                                , *module_version_it};
        std::map<unsigned int, downloading_module>::iterator
          iterator;
        bool inserted = false;
        boost::tie(iterator, inserted)
          = self->modules_downloading.insert(std::make_pair(*module_id_it, m));
        if(!inserted)
        {
          iterator->second.size = m.size;
          iterator->second.version = m.version;
          iterator->second.blocks_downloaded.resize(number_of_blocks);
          std::fill(iterator->second.blocks_downloaded.begin()
                    , iterator->second.blocks_downloaded.end(), 0);
          if(iterator->second.data.size() > m.size)
            iterator->second.data.resize(m.size);
        }
        else
          iterator->second.blocks_downloaded.resize(number_of_blocks);
      }

      // std::cout << "Total modules size: " << modules_size << std::endl;

      self->waiting_dii = false;
      if(!self->ddb_filter)
      {
        self->ddb_filter = 
          middleware_api_sections_create_filter_for_pid_and_table_id
          (pid, gts::constants::mpeg::dsmcc_types::download
           , &callbacks::section_callback
           , callbacks::section_callback_state
           (boost::bind(&ddb_filtering, self, pid, _1, _2, _3, callback)));
      }
#ifdef MIDDLEWARE_INTEGRATION_NORMATIVE_TRANSACTION_ID
      middleware_api_sections_create_filter_for_pid_and_table_id_and_table_id_extension
        (pid, gts::constants::mpeg::dsmcc_types::user_network
         , self->dsi_transaction_id.update_flag()?0:1
#else
      middleware_api_sections_create_filter_for_pid_and_table_id
        (pid, gts::constants::mpeg::dsmcc_types::user_network
#endif
         , &callbacks::section_callback
         , callbacks::section_callback_state
         (boost::bind(&dsi_update_filtering, self, pid, _1, _2, _3, callback)));

      middleware_api_sections_remove_filter(filter);
    }
  }
}

void dsi_filtering(dsmcc_filter* self, unsigned int pid, const char* buffer
                   , std::size_t size, middleware_api_sections_filter* filter
                   , boost::function<void()> callback)
{
  // std::cout << "DSI Filtering " << filter << std::endl;

  typedef gts::dsmcc::sections::user_network_message_section
    <char const*> section_un_type;
  section_un_type dsmcc_section(buffer, buffer + size);
  typedef section_un_type::user_network_message_iterator
    user_network_message_iterator;
  user_network_message_iterator user_network_message_it
    = gts::iterators::next<9>(dsmcc_section.begin());
  typedef user_network_message_iterator::deref_type user_network_message;
  user_network_message un = *user_network_message_it;
  typedef user_network_message::message_id_iterator message_id_iterator;
  message_id_iterator message_id_it = gts::iterators::next<2>(un.begin());
  if(*message_id_it == gts::dsmcc::download_message_id::download_server_initiate)
  {
    // std::cout << "======= Found DSI ========" << std::endl;
    update_dii(self, pid, filter, un, callback);
  }
}

}

void start(dsmcc_filter& dsmcc, unsigned int dsi_pid
           , boost::function<void()> callback)
{
  middleware_api_sections_filter* filter =
  middleware_api_sections_create_filter_for_pid_and_table_id
    (dsi_pid, gts::constants::mpeg::dsmcc_types::user_network
     , &callbacks::section_callback
     , callbacks::section_callback_state
     (boost::bind(&dsi_filtering, &dsmcc, dsi_pid, _1, _2, _3, callback)));
  // std::cout << "Filter returned " << filter << std::endl;        
}

void stop(dsmcc_filter&)
{
}

file_message const* find_file(dsmcc_filter& dsmcc, path const& p)
{
  typedef message_container::nth_index<1>::type index_type;
  typedef index_type::const_iterator index_iterator;

  index_type& index = dsmcc.messages.get<1>();
  
  index_iterator message_iterator = index.find(dsmcc.service_gateway_bind);
  assert(message_iterator != index.end());

  path::iterator path_first = p.begin(), path_last = p.end();
  while(path_first != path_last)
  {
    // std::cout << "Should search for |" << *path_first << '|' << std::endl;

    if(dsmcc::directory const* directory = boost::get<dsmcc::directory>
       (&message_iterator->variant))
    {
      std::vector<child_ref>::const_iterator child_first = directory->childs.begin()
        , child_last = directory->childs.end();
      while(child_first != child_last
            && (boost::distance(child_first->name)
                != boost::distance(*path_first)
                || !std::equal(boost::begin(*path_first)
                               , boost::end(*path_first)
                               , boost::begin(child_first->name))))
      {
        // std::cout << "Is |" << child_first->name << "| not what we look for"
        //           << std::endl;
        ++child_first;
      }
      if(child_first != child_last)
      {
        message_iterator = index.find(child_first->bind);
        if(message_iterator == index.end())
        {
          // std::cout << "Couldn't find bind in message_iterator" << std::endl;
          return 0;
        }
      }
      else
      {
        // std::cout << "Couldn't find childs with name " << *path_first << std::endl;
        return 0;
      }
    }
    else
    {
      // std::cout << "The element " << *path_first << " is not a directory" << std::endl;
      return 0;
    }

    ++path_first;
  }

  return boost::get<file_message const>(&message_iterator->variant);
}

std::pair<const char*, std::size_t> lock_file_content
 (dsmcc_filter& dsmcc, file_message const& file)
{
  typedef file_message::file_message_body_iterator
    file_message_body_iterator;

  typedef file_message_body_iterator::deref_type body_type;
  body_type body = *++ ++file.begin();
  
  typedef body_type::content_iterator content_iterator;
  typedef content_iterator::deref_type content_type;

  content_type content = *++body.begin();

  const char* buffer = &*content.begin();
  std::size_t size = boost::distance(content);
  return std::make_pair(buffer, size);
}

void unlock_file_content
 (dsmcc_filter& dsmcc, file_message const& file, const char* buffer, std::size_t size)
{
  
}

} }

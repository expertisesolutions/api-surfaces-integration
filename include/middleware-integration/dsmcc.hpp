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

#ifndef MIDDLEWARE_INTEGRATION_DSMCC_HPP
#define MIDDLEWARE_INTEGRATION_DSMCC_HPP

#include <middleware-integration/path.hpp>

#include <middleware-api/sections.h>

#include <gts/dsmcc/biop/message_header.hpp>
#include <gts/dsmcc/biop/message_subheader.hpp>
#include <gts/dsmcc/biop/directory_message.hpp>
#include <gts/dsmcc/biop/file_message.hpp>
#include <gts/dsmcc/transaction_id.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/function.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/optional.hpp>

#include <cstddef>

#include <map>
#include <vector>

namespace middleware_integration { namespace dsmcc {

#ifndef MIDDLEWARE_INTEGRATION_DISABLE_DSMCC_CONTROL
extern unsigned int allocation_size;

template <typename T>
struct allocator : std::allocator<T>
{
  template <typename U>
  struct rebind
  {
    typedef allocator<U> other;
  };

  typedef typename std::allocator<T>::pointer pointer;
  typedef typename std::allocator<T>::const_pointer const_pointer;
  typedef typename std::allocator<T>::size_type size_type;
  typedef typename std::allocator<void>::const_pointer void_const_pointer;

  pointer allocate(size_type n, void_const_pointer hint = 0)
  {
    pointer p = std::allocator<T>::allocate(n, hint);
    allocation_size += n;
    return p;
  }
  void deallocate(pointer p, size_type n)
  {
    std::allocator<T>::deallocate(p, n);
    allocation_size -= n;
  }
};
typedef std::vector<char, allocator<char> > buffer_container;
#else 
typedef std::vector<char> buffer_container;
#endif

#if !defined(MIDDLEWARE_INTEGRATION_DISABLE_DSMCC_CONTROL) \
    && !defined(MIDDLEWARE_INTEGRATION_SCARY_ASSIGNMENTS)
typedef std::vector<char, allocator<char> >::const_iterator dsmcc_iterator;
#else
typedef std::vector<char>::const_iterator dsmcc_iterator;
#endif

typedef gts::dsmcc::biop::directory_message<dsmcc_iterator> directory_message;
typedef gts::dsmcc::biop::file_message<dsmcc_iterator> file_message;
typedef gts::dsmcc::biop::bind<dsmcc_iterator> bind;

struct bind_info
{
  boost::iterator_range<dsmcc_iterator> object_key;
  unsigned int module_id;
};

struct child_ref
{
  boost::iterator_range<dsmcc_iterator> name;
  bind_info bind;
};

struct directory
{
  directory_message message;
  std::vector<child_ref> childs;
};

typedef boost::variant<directory, file_message> message_variant;

inline bool operator<(bind_info const& lhs, bind_info const& rhs)
{
  return lhs.module_id < rhs.module_id
         || (lhs.module_id == rhs.module_id && lhs.object_key < rhs.object_key);
}

struct message
{
  bind_info bind;
  message_variant variant;

  unsigned int module_id() const
  {
    return bind.module_id;
  }
};

typedef boost::multi_index_container
 <message
  , boost::multi_index::indexed_by
  <
      boost::multi_index::ordered_non_unique
      <boost::multi_index::const_mem_fun<message, unsigned int, &message::module_id> >
      , boost::multi_index::ordered_unique
      <boost::multi_index::member<message, bind_info, &message::bind> >
  >
> message_container;

struct downloaded_module
{
  unsigned int id;
  unsigned int version;

  boost::shared_ptr<buffer_container> data;
};

struct downloading_module
{
  unsigned int id;
  unsigned int size;
  unsigned int version;
  std::vector<int> blocks_downloaded;
  buffer_container data;

  bool finished() const
  {
    for(std::vector<int>::const_iterator
          first = blocks_downloaded.begin()
          , last = blocks_downloaded.end()
          ;first != last; ++first)
    {
      if(!*first)
        return false;
    }
    return true;
  }
};

struct dsmcc_filter
{
  dsmcc_filter()
    : waiting_dii(false)
    , service_gateway_module_id(0u)
    , download_id(0u), block_size(0u)
  {}

  bool waiting_dii;
  gts::dsmcc::transaction_id dii_transaction_id;
  gts::dsmcc::transaction_id dsi_transaction_id;
  unsigned int service_gateway_module_id;
  unsigned int download_id;
  unsigned int block_size;

  bind_info service_gateway_bind;

  message_container messages;
  std::map<unsigned int, downloaded_module> modules_downloaded;
  std::map<unsigned int, downloading_module> modules_downloading;
  boost::optional<middleware_api_sections_filter*> ddb_filter;
};

void start(dsmcc_filter& dsmcc, unsigned int dsi_pid, boost::function<void()> callback);
void stop(dsmcc_filter& dsmcc);

file_message const* find_file(dsmcc_filter& dsmcc, path const& p);

std::pair<const char*, std::size_t> lock_file_content
 (dsmcc_filter& dsmcc, file_message const& file);
void unlock_file_content
 (dsmcc_filter& dsmcc, file_message const& file, const char* buffer, std::size_t size);

struct content_lock
{
  dsmcc_filter& dsmcc;
  file_message const& file;
  const char* buffer;
  std::size_t size;

  content_lock(dsmcc_filter& dsmcc, file_message const& file)
    : dsmcc(dsmcc), file(file), buffer(0), size(0)
  {
    boost::tie(buffer, size) = lock_file_content(dsmcc, file);
  }
  ~content_lock()
  {
    unlock_file_content(dsmcc, file, buffer, size);
  }
};

} }

#endif

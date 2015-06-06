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

#ifndef MIDDLEWARE_INTEGRATION_PATH_HPP
#define MIDDLEWARE_INTEGRATION_PATH_HPP

#include <middleware-integration/log.hpp>
#include <boost/range/iterator_range.hpp>

#include <vector>
#include <stdexcept>

namespace middleware_integration {

namespace path_detail {

template <typename Iterator, typename OptionIterator>
Iterator find_one_of(Iterator first, Iterator last, OptionIterator option_start
                     , OptionIterator option_last)
{
  while(first != last)
  {
    OptionIterator option_first = option_start;
    while(option_first != option_last)
    {
      if(*first == *option_first)
        return first;
      else
        ++option_first;
    }
    ++first;
  }
  return first;
}

inline void fill_elements(std::vector<boost::iterator_range<const char*> >& elements
                          , boost::iterator_range<const char*> element)
{
  const char separators[] = {'/', '\\'};

  const char* first = boost::begin(element), *last = boost::end(element);
  while(first != last)
  {
    const char* dir_name_last = path_detail::find_one_of
      (first, last, &separators[0], &separators[0] + sizeof(separators));
    while(first == dir_name_last && first != last)
    {
      ++first;
      dir_name_last = path_detail::find_one_of
        (first, last, &separators[0], &separators[0] + sizeof(separators));
    }

    if(first != dir_name_last)
      elements.push_back(boost::make_iterator_range(first, dir_name_last));

    first = dir_name_last;
  }
}

}

struct path
{
  path(boost::iterator_range<const char*> element)
  {
    path_detail::fill_elements(elements, element);
  }
  path(std::string const& str)
  {
    if(!str.empty())
    {
      boost::iterator_range<const char*> range
        (&*str.begin(), &*str.begin() + str.size());
      path_detail::fill_elements(elements, range);
    }
  }

  typedef std::vector<boost::iterator_range<const char*> >::const_iterator const_iterator;
  typedef const_iterator iterator;

  const_iterator begin() const
  {
    return elements.begin();
  }
  const_iterator end() const
  {
    return elements.end();
  }
  path parent() const
  {
    assert(!elements.empty());
    path tmp(*this);
    tmp.elements.pop_back();
    return tmp;
  }
  std::string string() const
  {
    std::string r;
    for(std::vector<boost::iterator_range<const char*> >::const_iterator
          first = elements.begin(), last = elements.end()
          ;first != last; ++first)
    {
      r += '/';
      r.insert(r.end(), boost::begin(*first), boost::end(*first));
    }        
    return r;
  }

  std::vector<boost::iterator_range<const char*> > elements;
};

inline path operator/(path lhs, path rhs)
{
  static const char twodots[] = "..";
  while(!rhs.elements.empty()
        && boost::equal(rhs.elements[0]
                        , boost::make_iterator_range
                        (&twodots[0], &twodots[2])))
  {
    if(lhs.elements.empty())
      throw std::runtime_error("Path not existent");
    else
    {
      lhs.elements.pop_back();
      rhs.elements.erase(rhs.elements.begin());
    }
  }

  lhs.elements.insert(lhs.elements.end()
                      , rhs.elements.begin()
                      , rhs.elements.end());
  return lhs;
}

}

#endif

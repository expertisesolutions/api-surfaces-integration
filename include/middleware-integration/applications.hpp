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

#ifndef MIDDLEWARE_INTEGRATION_APPLICATIONS_HPP
#define MIDDLEWARE_INTEGRATION_APPLICATIONS_HPP

#include <middleware-integration/move.hpp>
#include <middleware-integration/dsmcc.hpp>
#include <middleware-integration/ncl.hpp>

#include <gts/constants/abnt/application_control_code.hpp>

#include <vector>
#include <map>

namespace middleware_integration { namespace applications {

struct application;
void swap(application& lhs, application& rhs);

struct application
{
  application(bool ncl, int application_id);

#ifdef MIDDLEWARE_INTEGRATION_MOVE_SUPPORT
  application(BOOST_RV_REF(application) p)
  {
    *this = p;
  }
#endif

  application()
    : ncl(false), application_id(0u)
    , control_code(gts::constants::abnt::application_control_codes::present)
  {}

  application& operator=(BOOST_COPY_ASSIGN_REF(application) p)
  {
    ncl = p.ncl;
    application_id = p.application_id;
    control_code = p.control_code;
    application_descriptor = p.application_descriptor;
    application_name_descriptor = p.application_name_descriptor;
    transport_protocol_descriptor = p.transport_protocol_descriptor;
    ginga_location_descriptor = p.ginga_location_descriptor;
    dsmcc = p.dsmcc;
    ncl_document = p.ncl_document;
    return *this;
  }

#ifdef MIDDLEWARE_INTEGRATION_MOVE_SUPPORT
  application& operator=(BOOST_RV_REF(application) p)
  {
    swap(p);
    return *this;
  }
#endif

  void download_complete();

  void swap(application& other)
  {
    applications::swap(*this, other);
  }

  bool ncl;
  unsigned int application_id
    , control_code;
  std::vector<char> application_descriptor
    , application_name_descriptor
    , transport_protocol_descriptor
    , ginga_location_descriptor;

  dsmcc::dsmcc_filter dsmcc;

  ncl::document ncl_document;

  BOOST_COPYABLE_AND_MOVABLE(application)
};

inline void swap(application& lhs, application& rhs)
{
  std::swap(lhs.ncl, rhs.ncl);
  std::swap(lhs.application_id, rhs.application_id);
  std::swap(lhs.control_code, rhs.control_code);
  std::swap(lhs.application_descriptor, rhs.application_descriptor);
  std::swap(lhs.application_name_descriptor, rhs.application_name_descriptor);
  std::swap(lhs.transport_protocol_descriptor, rhs.transport_protocol_descriptor);
  std::swap(lhs.ginga_location_descriptor, rhs.ginga_location_descriptor);
  std::swap(lhs.dsmcc, rhs.dsmcc);
  std::swap(lhs.ncl_document, rhs.ncl_document);
}

void add_application(application app);


} }

#endif

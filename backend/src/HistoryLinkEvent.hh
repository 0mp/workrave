// HistoryLinkEvent.hh --- An event of the Workrave core
//
// Copyright (C) 2007 Rob Caelers <robc@krandor.nl>
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// $Id$
//

#ifndef HISTORYLINKEVENT_HH
#define HISTORYLINKEVENT_HH

#include <string>
#include <list>

#include "LinkEvent.hh"
#include "ByteArray.hh"

using namespace workrave;

  //! Link Event
class HistoryLinkEvent : public LinkEvent
{
public:
  HistoryLinkEvent();
  HistoryLinkEvent(int size, guint8 *data);
  virtual ~HistoryLinkEvent();

  virtual std::string str() const;
  virtual std::string class_name() const;
  virtual void serialize(workrave::serialization::Target *s);

  int get_log_size() const
  {
    return log.size;
  }

  guint8 *get_log_data() const
  {
    return log.data;
  }

private:
  ByteArray log;
};

#endif // HISTORYLINKEVENT_HH

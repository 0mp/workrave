//
// Copyright (C) 2002, 2003, 2007, 2008, 2009, 2010, 2011, 2012 Rob Caelers <robc@krandor.nl>
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

#ifndef GIOSERVERSOCKET_HH
#define GIOSERVERSOCKET_HH

#if defined(HAVE_GIO_NET)

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "ISocketServer.hh"

//! Listen socket implementation using GIO
class GIOSocketServer
  : public ISocketServer
{
public:
  GIOSocketServer();
  virtual ~GIOSocketServer();

  // ISocketServer interface
  virtual void listen(int port);

private:
  static gboolean static_socket_incoming(GSocketService *service,
                                         GSocketConnection *connection,
                                         GObject *src_object,
                                         gpointer user_data);


private:
  GSocketService *service;
};

#endif
#endif // GIOSERVERSOCKET_HH

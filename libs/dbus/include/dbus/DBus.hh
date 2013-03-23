// DBus.hh --- DBUS interface
//
// Copyright (C) 2007, 2008, 2011, 2012, 2013 Rob Caelers <robc@krandor.nl>
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

#ifndef WORKRAVE_DBUS_DBUS_HH
#define WORKRAVE_DBUS_DBUS_HH

#if defined(PLATFORM_OS_WIN32_NATIVE)
#undef interface
#endif

#ifdef HAVE_DBUS
#ifdef HAVE_DBUS_GIO
#include "dbus/DBus-gio.hh"
#else
#include "dbus/DBus-freedesktop.hh"
#endif
#endif

#endif // WORKRAVE_DBUS_DBUS_HH

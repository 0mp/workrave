//
// Copyright (C) 2012 Rob Caelers <robc@krandor.nl>
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

#ifndef NETWORKINTERFACEMONITORBASE_HH
#define NETWORKINTERFACEMONITORBASE_HH

#include <map>

#include "NetworkInterfaceMonitor.hh"

//! 
class NetworkInterfaceMonitorBase : public NetworkInterfaceMonitor
{
public:
  virtual ~NetworkInterfaceMonitorBase() {}
  
  virtual sigc::signal<void, const NetworkInterfaceInfo &> &signal_interface_changed()
  {
    return interface_changed_signal;
  }

protected:
  sigc::signal<void, const NetworkInterfaceInfo &> interface_changed_signal;
};

#endif

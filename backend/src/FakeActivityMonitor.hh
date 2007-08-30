// FakeActivityMonitor.hh
//
// Copyright (C) 2001, 2002, 2003, 2005, 2006 Rob Caelers <robc@krandor.org>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id$
//

#ifndef FAKEACTIVITYMONITOR_HH
#define FAKEACTIVITYMONITOR_HH

#include "IActivityMonitor.hh"

class FakeActivityMonitor : public IActivityMonitor
{
public:
  FakeActivityMonitor() :
    suspended(false),
    state(ACTIVITY_IDLE)
  {
  }

  virtual ~FakeActivityMonitor() {}

  //! Stops the activity monitoring.
  void terminate()
  {
  }

  //! Suspends the activity monitoring.
  void suspend()
  {
    suspended = true;
  }

  //! Resumes the activity monitoring.
  void resume()
  {
    suspended = false;
  }

  //! Returns the current state
  ActivityState get_current_state()
  {
    if (suspended)
      {
        return ACTIVITY_SUSPENDED;
      }

    return state;
  }

  //! Force state to be idle.
  void force_idle()
  {
    state = ACTIVITY_IDLE;
  }

  void get_statistics(ActivityMonitorStatistics &stats) const
  {
    (void) stats;
  }

  void set_statistics(const ActivityMonitorStatistics &stats)
  {
    (void) stats;
  }

  void reset_statistics()
  {
  }

  void set_state(ActivityState s)
  {
    state = s;
  }

  void set_listener(ActivityMonitorListener *l)
  {
    (void)l;
  }

private:
  //! Monitor suspended?
  bool suspended;

  //! Current state
  ActivityState state;
};

#endif // FAKEACTIVITYMONITOR_HH

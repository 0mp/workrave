// TimerActivityMonitor.hh
//
// Copyright (C) 2001, 2002, 2003, 2004, 2005 Rob Caelers <robc@krandor.org>
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

#ifndef TIMERACTIVITYMONITOR_HH
#define TIMERACTIVITYMONITOR_HH

#include "ActivityMonitorInterface.hh"
#include "TimerInterface.hh"

//! An Activity Monitor that takes its activity state from a timer.
/*! This Activity Monitor is 'active' if the timer is running or if
 *  the timer has its idle time not at maximum, 'idle' otherwise.
 */
class TimerActivityMonitor : public ActivityMonitorInterface
{
public:
  //! Constructs an activity monitor that depends on specified timer.
  TimerActivityMonitor(Timer *t) :
    timer(t),
    suspended(false)
  {
  }

  virtual ~TimerActivityMonitor()
  {
  }
  
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
    
    Timer::TimerState state = timer->get_state();
    time_t idle = timer->get_elapsed_idle_time();
    time_t reset = timer->get_auto_reset();

    if (state == TimerInterface::STATE_STOPPED && idle >= reset)
      {
        return ACTIVITY_IDLE;
      }
    else
      {
        return ACTIVITY_ACTIVE;
      }
  }

  //! Force state to be idle.
  void force_idle()
  {
  }

  
  // Returns the collected statistics.
  void get_statistics(ActivityMonitorStatistics &stats) const
  {
    (void) stats;
  }


  //! Sets the statistics.
  void set_statistics(const ActivityMonitorStatistics &stats)
  {
    (void) stats;
  }


  //! Resets all collected statistics.
  void reset_statistics()
  {
  }

  
  //! Sets the activity listener of this monitor.
  void set_listener(ActivityMonitorListener *l)
  {
    (void)l;
  }
  
private:
  //! Reference timer.
  Timer *timer;

  //! Monitor suspended?
  bool suspended;
};

#endif // TIMERACTIVITYMONITOR_HH

// Control.hh --- The main controller
//
// Copyright (C) 2001, 2002 Rob Caelers <robc@krandor.org>
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

#ifndef CONTROL_HH
#define CONTROL_HH

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <iostream>
#include <string>

#include "ControlInterface.hh"
#include "ActivityMonitor.hh"

#include "Timer.hh"
#include "GUIInterface.hh"
#include "TimeSource.hh"
#include "Configurator.hh"
#include "ConfiguratorListener.hh"

class Control :
  public TimeSource,
  public ControlInterface,
  public ConfiguratorListener
{
public:
  typedef std::list<Timer *> Timers;
  typedef Timers::iterator TimerIter;
  typedef Timers::const_iterator TimerCIter;

  //! Contructor
  Control();

  //! Destructor
  virtual ~Control();

  //! The Main Entry point.
  int main(int argc, char **argv);

  // TimeSource methods
  time_t get_time() const;

  // ControlInterface methods.
  void init();
  void terminate();
  void process_timers(map<string, TimerInfo> &infos);

#ifndef NDEBUG
  void test_me();
#endif
  
  //! Returns all timers (to be removed as public ??)
  Timers get_timers() const
  {
    return timers;
  }

  ActivityMonitor *get_activity_monitor() const
  {
    return monitor;
  }

  list<string> get_timer_ids() const;
  TimerInterface *get_timer(string id);

  virtual void config_changed_notify(string key);


private:
  void save_state() const;
  void load_state();

  void configure_timer(string id, Timer *timer);
  void configure_timer_monitor(string id, Timer *timer);
  void load_monitor_config();
  void store_monitor_config();
  bool create_monitor();
  bool create_timers();
  //bool process_timer_event(Timer *timer, Timer::TimerEvent event);
  
private:
  //! List of timers
  Timers timers;

  // COnfiguration access.
  Configurator *configurator;

  //! The activity monitor
  ActivityMonitor *monitor;

  //! The current time.
  time_t current_time;
};

#endif // CONTROL_HH

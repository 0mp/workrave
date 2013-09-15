// LocalActivityMonitor.hh --- LocalActivityMonitor functionality
//
// Copyright (C) 2001, 2002, 2003, 2004, 2006, 2007, 2010, 2012, 2013 Rob Caelers <robc@krandor.nl>
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

#ifndef LOCALACTIVITYMONITOR_HH
#define LOCALACTIVITYMONITOR_HH

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "config/Config.hh"
#include "input-monitor/IInputMonitor.hh"
#include "input-monitor/IInputMonitorListener.hh"

using namespace workrave::config;
using namespace workrave::input_monitor;

class IActivityMonitorListener
{
public:
  typedef boost::shared_ptr<IActivityMonitorListener> Ptr;

  virtual ~IActivityMonitorListener() {}

  // Notification that the user is currently active.
  virtual bool action_notify() = 0;
};

class LocalActivityMonitor :
  public IInputMonitorListener,
  public IConfiguratorListener
{
public:
  typedef boost::shared_ptr<LocalActivityMonitor> Ptr;

public:
  static Ptr create(IConfigurator::Ptr configurator, const std::string &display_name);
  
  LocalActivityMonitor(IConfigurator::Ptr configurator, const std::string &display_name);
  virtual ~LocalActivityMonitor();

  void init();
  void terminate();
  void suspend();
  void resume();
  void force_idle();
  bool is_active();
  void set_listener(IActivityMonitorListener::Ptr l);

  // IInputMonitorListener
  virtual void action_notify();
  virtual void mouse_notify(int x, int y, int wheel = 0);
  virtual void button_notify(bool is_press);
  virtual void keyboard_notify(bool repeat);
  
private:
  void call_listener();

  void load_config();
  void config_changed_notify(const std::string &key);
  void set_parameters(int noise, int activity, int idle);
  void get_parameters(int &noise, int &activity, int &idle);

  void process_state();
  
  //! State of the activity monitor.
  enum LocalActivityMonitorState
    {
      ACTIVITY_MONITOR_UNKNOWN,
      ACTIVITY_MONITOR_SUSPENDED,
      ACTIVITY_MONITOR_IDLE,
      ACTIVITY_MONITOR_FORCED_IDLE,
      ACTIVITY_MONITOR_NOISE,
      ACTIVITY_MONITOR_ACTIVE
    };
  
private:
  //! The Configurator.
  IConfigurator::Ptr configurator;

  //! 
  std::string display_name;
  
  //! The actual monitoring driver.
  IInputMonitor::Ptr input_monitor;

  //! the current state.
  LocalActivityMonitorState state;

  //! Internal locking
  boost::recursive_mutex lock;

  //! Previous X coordinate
  int prev_x;

  //! Previous Y coordinate
  int prev_y;

  //! Is the button currently pressed?
  bool button_is_pressed;

  //! Last time activity was detected
  int64_t last_action_time;

  //! First time the \c ACTIVITY_IDLE state was left.
  int64_t first_action_time;

  //! The noise threshold
  int64_t noise_threshold;

  //! The activity threshold.
  int64_t activity_threshold;

  //! The idle threshold.
  int64_t idle_threshold;

  //! Activity listener.
  IActivityMonitorListener::Ptr listener;
};

#endif // LOCALACTIVITYMONITOR_HH

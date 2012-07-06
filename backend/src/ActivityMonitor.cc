//
// Copyright (C) 2001 - 2010, 2012 Rob Caelers <robc@krandor.nl>
// Copyright (C) 2007 Ray Satiro <raysatiro@yahoo.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ActivityMonitor.hh"

#include <assert.h>
#include <math.h>

#include <stdio.h>
#include <sys/types.h>
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

#include "input-monitor/IInputMonitor.hh"
#include "input-monitor/InputMonitorFactory.hh"

#include "IActivityMonitorListener.hh"
#include "CoreConfig.hh"
#include "debug.hh"
#include "timeutil.h"

using namespace std;

ActivityMonitor::Ptr
ActivityMonitor::create(IConfigurator::Ptr configurator)
{
  return Ptr(new ActivityMonitor(configurator));
}

    
//! Constructor.
ActivityMonitor::ActivityMonitor(IConfigurator::Ptr configurator) :
  configurator(configurator),
  activity_state(ACTIVITY_IDLE),
  prev_x(-10),
  prev_y(-10),
  click_x(-1),
  click_y(-1),
  button_is_pressed(false)
{
  TRACE_ENTER("ActivityMonitor::ActivityMonitor");

  first_action_time.tv_sec = 0;
  first_action_time.tv_usec = 0;

  last_action_time.tv_sec = 0;
  last_action_time.tv_usec = 0;

  noise_threshold.tv_sec = 1;
  noise_threshold.tv_usec = 0;

  activity_threshold.tv_sec = 2;
  activity_threshold.tv_usec = 0;

  idle_threshold.tv_sec = 5;
  idle_threshold.tv_usec = 0;

  TRACE_EXIT();
}


//! Destructor.
ActivityMonitor::~ActivityMonitor()
{
  TRACE_ENTER("ActivityMonitor::~ActivityMonitor");
  delete input_monitor;
  TRACE_EXIT();
}


//! Initializes the monitor.
void
ActivityMonitor::init(const string &display_name)
{
  TRACE_ENTER("ActivityMonitor::init");

  InputMonitorFactory::init(configurator, display_name);

  load_config();
  configurator->add_listener(CoreConfig::CFG_KEY_MONITOR, this);

  input_monitor = InputMonitorFactory::get_monitor(IInputMonitorFactory::CAPABILITY_ACTIVITY);
  if (input_monitor != NULL)
    {
      input_monitor->subscribe(this);
    }
  
  TRACE_EXIT();
}


//! Terminates the monitor.
void
ActivityMonitor::terminate()
{
  TRACE_ENTER("ActivityMonitor::terminate");

  if (input_monitor != NULL)
    {
      input_monitor->terminate();
    }

  TRACE_EXIT();
}


//! Suspends the activity monitoring.
void
ActivityMonitor::suspend()
{
  TRACE_ENTER_MSG("ActivityMonitor::suspend", activity_state);
  lock.lock();
  activity_state = ACTIVITY_SUSPENDED;
  lock.unlock();
  TRACE_RETURN(activity_state);
}


//! Resumes the activity monitoring.
void
ActivityMonitor::resume()
{
  TRACE_ENTER_MSG("ActivityMonitor::resume", activity_state);
  lock.lock();
  activity_state = ACTIVITY_IDLE;
  lock.unlock();
  TRACE_RETURN(activity_state);
}


//! Forces state te be idle.
void
ActivityMonitor::force_idle()
{
  TRACE_ENTER_MSG("ActivityMonitor::force_idle", activity_state);
  lock.lock();
  if (activity_state != ACTIVITY_SUSPENDED)
    {
      activity_state = ACTIVITY_IDLE;
      last_action_time.tv_sec = 0;
      last_action_time.tv_usec = 0;
    }
  lock.unlock();
  TRACE_RETURN(activity_state);
}


//! Returns the current state
ActivityState
ActivityMonitor::get_current_state()
{
  TRACE_ENTER_MSG("ActivityMonitor::get_current_state", activity_state);
  lock.lock();

  // First update the state...
  if (activity_state == ACTIVITY_ACTIVE)
    {
      GTimeVal now, tv;

      g_get_current_time(&now);
      tvSUBTIME(tv, now, last_action_time);

      TRACE_MSG("Active: "
                << tv.tv_sec << "." << tv.tv_usec << " "
                << idle_threshold.tv_sec << " " << idle_threshold.tv_usec);
      if (tvTIMEGT(tv, idle_threshold))
        {
          // No longer active.
          activity_state = ACTIVITY_IDLE;
        }
    }

  lock.unlock();
  TRACE_RETURN(activity_state);
  return activity_state;
}


//! Sets the operation parameters.
void
ActivityMonitor::set_parameters(int noise, int activity, int idle)
{
  noise_threshold.tv_sec = noise / 1000;
  noise_threshold.tv_usec = (noise % 1000) * 1000;

  activity_threshold.tv_sec = activity / 1000;
  activity_threshold.tv_usec = (activity % 1000) * 1000;

  idle_threshold.tv_sec = idle / 1000;
  idle_threshold.tv_usec = (idle % 1000) * 1000;

  // The easy way out.
  activity_state = ACTIVITY_IDLE;
}


//! Sets the operation parameters.
void
ActivityMonitor::get_parameters(int &noise, int &activity, int &idle)
{
  noise = noise_threshold.tv_sec * 1000 + noise_threshold.tv_usec / 1000;
  activity = activity_threshold.tv_sec * 1000 + activity_threshold.tv_usec / 1000;
  idle = idle_threshold.tv_sec * 1000 + idle_threshold.tv_usec / 1000;
}


//! Shifts the internal time (after system clock has been set)
void
ActivityMonitor::shift_time(int delta)
{
  GTimeVal d;

  lock.lock();

  tvSETTIME(d, delta, 0)

  if (!tvTIMEEQ0(last_action_time))
    tvADDTIME(last_action_time, last_action_time, d);

  if (!tvTIMEEQ0(first_action_time))
    tvADDTIME(first_action_time, first_action_time, d);

  lock.unlock();
}


//! Sets the callback listener.
void
ActivityMonitor::set_listener(IActivityMonitorListener::Ptr l)
{
  lock.lock();
  listener = l;
  lock.unlock();
}


//! Activity is reported by the input monitor.
void
ActivityMonitor::action_notify()
{
  lock.lock();

  GTimeVal now;
  g_get_current_time(&now);

  switch (activity_state)
    {
    case ACTIVITY_IDLE:
      {
        first_action_time = now;
        last_action_time = now;

        if (tvTIMEEQ0(activity_threshold))
          {
            activity_state = ACTIVITY_ACTIVE;
          }
        else
          {
            activity_state = ACTIVITY_NOISE;
          }
      }
      break;

    case ACTIVITY_NOISE:
      {
        GTimeVal tv;

        tvSUBTIME(tv, now, last_action_time);
        if (tvTIMEGT(tv, noise_threshold))
          {
            first_action_time = now;
          }
        else
          {
            tvSUBTIME(tv, now, first_action_time);
            if (tvTIMEGEQ(tv, activity_threshold))
              {
                activity_state = ACTIVITY_ACTIVE;
              }
          }
      }
      break;

    default:
      break;
    }

  last_action_time = now;
  lock.unlock();
  call_listener();
}


//! Mouse activity is reported by the input monitor.
void
ActivityMonitor::mouse_notify(int x, int y, int wheel_delta)
{
  static const int sensitivity = 3;

  lock.lock();
  const int delta_x = x - prev_x;
  const int delta_y = y - prev_y;
  prev_x = x;
  prev_y = y;

  if (abs(delta_x) >= sensitivity || abs(delta_y) >= sensitivity
      || wheel_delta != 0 || button_is_pressed)
    {
      action_notify();
    }
  lock.unlock();
}


//! Mouse button activity is reported by the input monitor.
void
ActivityMonitor::button_notify(bool is_press)
{
  lock.lock();

  button_is_pressed = is_press;

  if (is_press)
    {
      action_notify();
    }

  lock.unlock();
}


//! Keyboard activity is reported by the input monitor.
void
ActivityMonitor::keyboard_notify(bool repeat)
{
  (void)repeat;

  lock.lock();
  action_notify();
  lock.unlock();
}


//! Calls the callback listener.
void
ActivityMonitor::call_listener()
{
  IActivityMonitorListener::Ptr l;

  lock.lock();
  l = listener;
  lock.unlock();

  if (l != NULL)
    {
      // Listener is set.
      if (!l->action_notify())
        {
          // Remove listener.
          lock.lock();
          listener.reset();
          lock.unlock();
        }
    }
}

//! Notification that the configuration has changed.
void
ActivityMonitor::config_changed_notify(const string &key)
{
  TRACE_ENTER_MSG("ActivityMonitor::config_changed_notify", key);
  string::size_type pos = key.find('/');
  string path;

  if (pos != string::npos)
    {
      path = key.substr(0, pos);
    }

  if (path == CoreConfig::CFG_KEY_MONITOR)
    {
      load_config();
    }
  TRACE_EXIT();
}


//! Loads the configuration of the monitor.
void
ActivityMonitor::load_config()
{
  TRACE_ENTER("ActivityMonitor::load_config");

  int noise;
  int activity;
  int idle;

  assert(configurator != NULL);

  if (! configurator->get_value(CoreConfig::CFG_KEY_MONITOR_NOISE, noise))
    noise = 9000;
  if (! configurator->get_value(CoreConfig::CFG_KEY_MONITOR_ACTIVITY, activity))
    activity = 1000;
  if (! configurator->get_value(CoreConfig::CFG_KEY_MONITOR_IDLE, idle))
    idle = 5000;

  // Pre 1.0 compatibility...
  if (noise < 50)
    {
      noise *= 1000;
      configurator->set_value(CoreConfig::CFG_KEY_MONITOR_NOISE, noise);
    }

  if (activity < 50)
    {
      activity *= 1000;
      configurator->set_value(CoreConfig::CFG_KEY_MONITOR_ACTIVITY, activity);
    }

  if (idle < 50)
    {
      idle *= 1000;
      configurator->set_value(CoreConfig::CFG_KEY_MONITOR_IDLE, idle);
    }

  TRACE_MSG("Monitor config = " << noise << " " << activity << " " << idle);

  set_parameters(noise, activity, idle);
  TRACE_EXIT();
}

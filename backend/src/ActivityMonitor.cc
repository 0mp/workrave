// ActivityMonitor.cc --- ActivityMonitor
//
// Copyright (C) 2001 - 2007 Rob Caelers <robc@krandor.org>
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

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ActivityMonitor.hh"
#include "ActivityMonitorListener.hh"

#include "debug.hh"
#include "timeutil.h"
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

#if defined(HAVE_X)
#include "X11InputMonitor.hh"
#elif defined(WIN32)
#include "CoreFactory.hh"
#include "IConfigurator.hh"
#include "W32InputMonitor.hh"
#include "W32AlternateMonitor.hh"
#endif


//! Constructor.
ActivityMonitor::ActivityMonitor(const char *display) :
  activity_state(ACTIVITY_IDLE),
  prev_x(-10),
  prev_y(-10),
  click_x(-1),
  click_y(-1),
  button_is_pressed(false),
  listener(NULL)
{
  TRACE_ENTER("ActivityMonitor::ActivityMonitor");

  (void) display;

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

  last_mouse_time.tv_sec = 0;
  last_mouse_time.tv_usec = 0;

  total_mouse_time.tv_sec = 0;
  total_mouse_time.tv_usec = 0;

  statistics.total_movement = 0;
  statistics.total_click_movement = 0;
  statistics.total_clicks = 0;
  statistics.total_keystrokes = 0;

#if defined(HAVE_X)

  input_monitor = new X11InputMonitor(display);
  input_monitor->init( this );

#elif defined(WIN32)

  bool nohooks;
  CoreFactory::get_configurator()->get_value_default( "advanced/nohooks", &nohooks, false );

  if (!nohooks)
    {
      input_monitor = new W32InputMonitor();

      if( !input_monitor->init( this ) )
        {
          TRACE_MSG( "W32InputMonitor failed to initialize." );

          if( LOBYTE( LOWORD( GetVersion() ) ) < 5 )
            // If < Win2k, there is no alternate monitor available.
            {
              MessageBoxA( NULL,
                           "Workrave must be able to monitor certain system "
                           "events in order to determine when you are idle.\n\n"

                           "An attempt was made to hook into your system, but it "
                           "was unsuccessful.\n\n"
                           "Workrave must exit now.\n",
                           "Workrave: Debug Message",
                           MB_OK | MB_ICONERROR | MB_TOPMOST );
              /*
                We want the current input monitor destructor called
                at this point, after the user responds to the messagebox.
                This way, if debugging, debug output can be viewed before
                the user responds to the message box.
              */
              delete input_monitor;

              CoreFactory::get_configurator()->save();
              TRACE_EXIT();
              exit( 0 );
            }
          else
            // request to start the alternate monitor
            {
              int ret = MessageBoxA( NULL,
                                     "Workrave must be able to monitor certain system "
                                     "events in order to determine when you are idle.\n\n"

                                     "An attempt was made to hook into your system, but it "
                                     "was unsuccessful.\n\n"

                                     "You might have system safety software that blocks "
                                     "Workrave from installing global hooks on your system.\n\n"

                                     "You can instead run Workrave using the alternate monitor, "
                                     "which doesn't require global hooks.\n\n"

                                     "Click 'OK' to run the alternate monitor, or 'Cancel' to exit.\n",
                                     "Workrave: Debug Message",
                                     MB_OKCANCEL | MB_ICONSTOP | MB_TOPMOST );
              /*
                We want the current input monitor destructor called
                at this point, after the user responds to the messagebox.
                This way, if debugging, debug output can be viewed before
                the user responds to the message box.
              */
              delete input_monitor;

              if( ret == IDCANCEL )
                {
                  CoreFactory::get_configurator()->save();
                  TRACE_EXIT();
                  exit( 0 );
                }
              else
                {
                  nohooks = true;
                  CoreFactory::get_configurator()->set_value( "advanced/nohooks", true );
                }
            }
        }
    }

  if( nohooks )
    {
      input_monitor = new W32AlternateMonitor();

      if( !input_monitor->init( this ) )
        {
          TRACE_MSG( "W32AlternateMonitor failed to initialize." );

          delete input_monitor;

          if( LOBYTE( LOWORD( GetVersion() ) ) < 5 )
            {
              MessageBoxA( NULL,
                           "Workrave's alternate activity monitor failed to initialize.\n\n"

                           "The alternate activity monitor is not compatible with your "
                           "version of Windows.\n\n"

                           "Workrave will use the regular activity monitor from now on.\n\n"

                           "Workrave must exit now. Restart it for changes to take effect.\n",
                           "Workrave: Debug Message",
                           MB_OK | MB_ICONERROR | MB_TOPMOST );

              CoreFactory::get_configurator()->set_value( "advanced/nohooks", false );

              CoreFactory::get_configurator()->save();
              TRACE_EXIT();
              exit( 0 );
            }
          else
            {
              int ret = MessageBoxA( NULL,
                                     "Workrave's alternate activity monitor failed to initialize.\n\n"

                                     "Workrave must exit now.\n\n"

                                     "The next time you start Workrave it can use the regular "
                                     "activity monitor instead.\n\n"

                                     "Click 'OK' to use the regular activity monitor next time.\n",
                                     "Workrave: Debug Message",
                                     MB_OKCANCEL | MB_ICONERROR | MB_TOPMOST );

              if( ret == IDOK )
                CoreFactory::get_configurator()->set_value( "advanced/nohooks", false );

              CoreFactory::get_configurator()->save();
              TRACE_EXIT();
              exit( 0 );
            }
        }
    }
#endif

  TRACE_EXIT();
}


//! Destructor.
ActivityMonitor::~ActivityMonitor()
{
  TRACE_ENTER("ActivityMonitor::~ActivityMonitor");

  delete input_monitor;

  TRACE_EXIT();
}


//! Terminates the monitor.
void
ActivityMonitor::terminate()
{
  TRACE_ENTER("ActivityMonitor::terminate");

  input_monitor->terminate();

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
      struct timeval now, tv;
      gettimeofday(&now, NULL);

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


//! Returns the statistics.
void
ActivityMonitor::get_statistics(ActivityMonitorStatistics &stats) const
{
  stats = statistics;
  stats.total_movement_time = total_mouse_time.tv_sec;
}


//! Sets the statistics
void
ActivityMonitor::set_statistics(const ActivityMonitorStatistics &stats)
{
  statistics = stats;
  total_mouse_time.tv_sec = stats.total_movement_time;
  total_mouse_time.tv_usec = 0;
}


//! Resets the statistics.
void
ActivityMonitor::reset_statistics()
{
  total_mouse_time.tv_sec = 0;
  total_mouse_time.tv_usec = 0;

  statistics.total_movement = 0;
  statistics.total_click_movement = 0;
  statistics.total_clicks = 0;
  statistics.total_keystrokes = 0;
}



//! Shifts the internal time (after system clock has been set)
void
ActivityMonitor::shift_time(int delta)
{
  struct timeval d;

  lock.lock();
  tvSETTIME(d, delta, 0);

  if (!tvTIMEEQ0(last_action_time))
    tvADDTIME(last_action_time, last_action_time, d);

  if (!tvTIMEEQ0(first_action_time))
    tvADDTIME(first_action_time, first_action_time, d);

  if (!tvTIMEEQ0(last_mouse_time))
    tvADDTIME(last_mouse_time, last_mouse_time, d);
  lock.unlock();
}


//! Sets the callback listener.
void
ActivityMonitor::set_listener(ActivityMonitorListener *l)
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

  struct timeval now;
  gettimeofday(&now, NULL);

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
        struct timeval tv;

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
      statistics.total_movement += int(sqrt((double)(delta_x * delta_x + delta_y * delta_y)));

      action_notify();

      struct timeval now, tv;

      gettimeofday(&now, NULL);
      tvSUBTIME(tv, now, last_mouse_time);

      if (!tvTIMEEQ0(last_mouse_time) && tv.tv_sec < 1 && tv.tv_sec >= 0 && tv.tv_usec >= 0)
        {
          tvADDTIME(total_mouse_time, total_mouse_time, tv);
        }

      last_mouse_time = now;

    }
  lock.unlock();
}


//! Mouse button activity is reported by the input monitor.
void
ActivityMonitor::button_notify(int button_mask, bool is_press)
{
  (void)button_mask;

  lock.lock();
  if (click_x != -1)
    {
      int delta_x = click_x - prev_x;
      int delta_y = click_y - prev_y;

      statistics.total_click_movement += int(sqrt((double)(delta_x * delta_x + delta_y * delta_y)));
    }

  click_x = prev_x;
  click_y = prev_y;

  button_is_pressed = is_press;

  if (is_press)
    {
      action_notify();
      statistics.total_clicks++;
    }

  lock.unlock();
}


//! Keyboard activity is reported by the input monitor.
void
ActivityMonitor::keyboard_notify(int key_code, int modifier)
{
  (void)key_code;
  (void)modifier;

  lock.lock();
  action_notify();
  statistics.total_keystrokes++;
  lock.unlock();
}


//! Calls the callback listener.
void
ActivityMonitor::call_listener()
{
  ActivityMonitorListener *l = NULL;

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
          listener = NULL;
          lock.unlock();
        }
    }
}

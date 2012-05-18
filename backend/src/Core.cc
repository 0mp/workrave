// Core.cc --- The main controller
//
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Rob Caelers & Raymond Penners
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

#include "debug.hh"

#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Core.hh"

#include "Util.hh"
#include "IApp.hh"
#include "ICoreEventListener.hh"
#include "ActivityMonitor.hh"
#include "TimerActivityMonitor.hh"
#include "Break.hh"
#include "ConfiguratorFactory.hh"
#include "IConfigurator.hh"
#include "CoreConfig.hh"
#include "Statistics.hh"
#include "BreakControl.hh"
#include "Timer.hh"
#include "InputMonitorFactory.hh"
#include "TimeSource.hh"

#ifdef HAVE_TESTS
#include "FakeActivityMonitor.hh"
#endif

#ifdef HAVE_GCONF
#include <gconf/gconf-client.h>
#endif

#ifdef HAVE_DBUS
#if defined(PLATFORM_OS_WIN32_NATIVE)
#undef interface
#endif
#include "DBus.hh"
#include "DBusException.hh"
#ifdef HAVE_TESTS
#include "Test.hh"
#endif
#endif

#ifdef HAVE_BROKEN_DISTRIBUTION
#include "Network.hh"
#endif

Core *Core::instance = NULL;

const char *WORKRAVESTATE="WorkRaveState";
const int SAVESTATETIME = 60;

#define DBUS_PATH_WORKRAVE         "/org/workrave/Workrave/Core"
#define DBUS_SERVICE_WORKRAVE      "org.workrave.Workrave"

//! Constructs a new Core.
Core::Core() :
  last_process_time(0),
  monitor(NULL),
  application(NULL),
  statistics(NULL),
  operation_mode(OPERATION_MODE_NORMAL),
  operation_mode_regular(OPERATION_MODE_NORMAL),
  usage_mode(USAGE_MODE_NORMAL),
  core_event_listener(NULL),
  powersave(false),
  powersave_resume_time(0),
  insist_policy(ICore::INSIST_POLICY_HALT),
  active_insist_policy(ICore::INSIST_POLICY_INVALID),
  resume_break(BREAK_ID_NONE),
  local_state(ACTIVITY_IDLE),
  monitor_state(ACTIVITY_UNKNOWN)
{
  TRACE_ENTER("Core::Core");
  current_time = time(NULL);
  TimeSource::source = this;
  
  assert(! instance);
  instance = this;

#ifdef HAVE_TESTS
  fake_monitor = NULL;
  manual_clock = false;

  const char *env = getenv("WORKRAVE_TEST");
  if (env != NULL)
    {
      current_time = 1000;
      manual_clock = true;
    }
#endif

  TRACE_EXIT();
}


//! Destructor.
Core::~Core()
{
  TRACE_ENTER("Core::~Core");

  save_state();

  if (monitor != NULL)
    {
      monitor->terminate();
    }

  delete statistics;
  delete monitor;

#ifdef HAVE_TESTS
  delete fake_monitor;
#endif

#ifdef HAVE_BROKEN_DISTRIBUTION
  if (network != NULL)
    {
      network->terminate();
    }
  delete network;
#endif

  TRACE_EXIT();
}


/********************************************************************************/
/**** Initialization                                                       ******/
/********************************************************************************/


//! Initializes the core.
void
Core::init(int argc, char **argv, IApp *app, const string &display_name)
{
  application = app;
  this->argc = argc;
  this->argv = argv;

  init_configurator();
  init_monitor(display_name);
  init_breaks();
  init_statistics();
  init_networking();
  init_bus();

  load_state();
  load_misc();
}


//! Initializes the configurator.
void
Core::init_configurator()
{
  string ini_file = Util::complete_directory("workrave.ini", Util::SEARCH_PATH_CONFIG);

  if (Util::file_exists(ini_file))
    {
      configurator = ConfiguratorFactory::create(ConfiguratorFactory::FormatIni);
      configurator->load(ini_file);
    }
  else
    {
#if defined(HAVE_GCONF)
      gconf_init(argc, argv, NULL);
      g_type_init();
#endif
      
      configurator = ConfiguratorFactory::create(ConfiguratorFactory::FormatNative);
#if defined(HAVE_GDOME)
      if (configurator == NULL)
        {
          string configFile = Util::complete_directory("config.xml", Util::SEARCH_PATH_CONFIG);
          configurator = ConfiguratorFactory::create(ConfiguratorFactory::FormatXml);

#  if defined(PLATFORM_OS_UNIX)
          if (configFile == "" || configFile == "config.xml")
            {
              configFile = Util::get_home_directory() + "config.xml";
            }
#  endif
          if (configFile != "")
            {
              configurator->load(configFile);
            }
        }
#endif
      if (configurator == NULL)
        {
          ini_file = Util::get_home_directory() + "workrave.ini";
          configurator = ConfiguratorFactory::create(ConfiguratorFactory::FormatIni);
          configurator->load(ini_file);
          configurator->save(ini_file);
        }
    }
  
  string home;
  if (configurator->get_value(CoreConfig::CFG_KEY_GENERAL_DATADIR, home) &&
      home != "")
    {
      Util::set_home_directory(home);
    }
}

//! Initializes the communication bus.
void
Core::init_bus()
{
#ifdef HAVE_DBUS
  try
    {
      dbus = new DBus();
      dbus->init();

      extern void init_DBusWorkrave(DBus *dbus);
      init_DBusWorkrave(dbus);

      dbus->connect(DBUS_PATH_WORKRAVE, "org.workrave.CoreInterface", this);
      dbus->connect(DBUS_PATH_WORKRAVE, "org.workrave.ConfigInterface", configurator.get());
      dbus->register_object_path(DBUS_PATH_WORKRAVE);
      
#ifdef HAVE_TESTS
      dbus->connect("/org/workrave/Workrave/Debug", "org.workrave.DebugInterface", Test::get_instance());
      dbus->register_object_path("/org/workrave/Workrave/Debug");
#endif
    }
  catch (DBusException &)
    {
    }
#endif
}


//! Initializes the network facility.
void
Core::init_networking()
{
#ifdef HAVE_BROKEN_DISTRIBUTION  
  network = new Network();
  network->init(27273);

  network->subscribe("breaklinkevent", this);
  network->subscribe("corelinkevent", this);
  network->subscribe("timerstatelinkevent", this);
  
  network->report_active(false);
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      network->report_timer_state(i, false);
    }
#endif
}


//! Initializes the activity monitor.
void
Core::init_monitor(const string &display_name)
{
#ifdef HAVE_TESTS
  fake_monitor = NULL;
  const char *env = getenv("WORKRAVE_FAKE");
  if (env != NULL)
    {
      fake_monitor = new FakeActivityMonitor();
    }
#endif

  InputMonitorFactory::init(display_name);

  monitor = new ActivityMonitor();
  load_monitor_config();

  configurator->add_listener(CoreConfig::CFG_KEY_MONITOR, this);
}


//! Initializes all breaks.
void
Core::init_breaks()
{
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      breaks[i].init(BreakId(i), application);
    }
  application->set_break_response(this);
}


//! Initializes the statistics.
void
Core::init_statistics()
{
  statistics = new Statistics();
  statistics->init(this);
}


//! Loads the configuration of the monitor.
void
Core::load_monitor_config()
{
  TRACE_ENTER("Core::load_monitor_config");

  int noise;
  int activity;
  int idle;

  assert(configurator != NULL);
  assert(monitor != NULL);

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

  monitor->set_parameters(noise, activity, idle);
  TRACE_EXIT();
}


//! Notification that the configuration has changed.
void
Core::config_changed_notify(const string &key)
{
  TRACE_ENTER_MSG("Core::config_changed_notify", key);
  string::size_type pos = key.find('/');
  string path;

  if (pos != string::npos)
    {
      path = key.substr(0, pos);
    }

  if (path == CoreConfig::CFG_KEY_MONITOR)
    {
      load_monitor_config();
    }

  if (key == CoreConfig::CFG_KEY_OPERATION_MODE)
    {
      int mode;
      if (! get_configurator()->get_value(CoreConfig::CFG_KEY_OPERATION_MODE, mode))
        {
          mode = OPERATION_MODE_NORMAL;
        }
      TRACE_MSG("Setting operation mode");
      set_operation_mode_internal(OperationMode(mode), false);
    }

  if (key == CoreConfig::CFG_KEY_USAGE_MODE)
    {
      int mode;
      if (! get_configurator()->get_value(CoreConfig::CFG_KEY_USAGE_MODE, mode))
        {
          mode = USAGE_MODE_NORMAL;
        }
      TRACE_MSG("Setting usage mode");
      set_usage_mode_internal(UsageMode(mode), false);
    }
  TRACE_EXIT();
}


/********************************************************************************/
/**** TimeSource interface                                                 ******/
/********************************************************************************/

//! Retrieve the current time.
time_t
Core::get_time() const
{
  return current_time;
}


/********************************************************************************/
/**** Core Interface                                                       ******/
/********************************************************************************/

//! Returns the specified timer.
Timer *
Core::get_timer(BreakId id) const
{
  if (id >= 0 && id < BREAK_ID_SIZEOF)
    {
      return breaks[id].get_timer();
    }
  else
    {
      return NULL;
    }
}


//! Returns the specified timer.
Timer *
Core::get_timer(string name) const
{
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      if (breaks[i].get_name() == name)
        {
          return breaks[i].get_timer();
        }
    }
  return NULL;
}


//! Returns the configurator.
IConfigurator::Ptr
Core::get_configurator() const
{
  return configurator;
}


//! Returns the activity monitor.
IActivityMonitor *
Core::get_activity_monitor() const
{
  return monitor;
}


//! Returns the statistics.
Statistics *
Core::get_statistics() const
{
  return statistics;
}


//! Returns the specified break controller.
Break *
Core::get_break(BreakId id)
{
  assert(id >= 0 && id < BREAK_ID_SIZEOF);
  return &breaks[id];
}

//! Returns the specified break controller.
Break *
Core::get_break(std::string name)
{
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      if (breaks[i].get_name() == name)
        {
          return &breaks[i];
        }
    }
  return NULL;
}


//! Retrieves the operation mode.
OperationMode
Core::get_operation_mode()
{
    return operation_mode;
}

//! Retrieves the regular operation mode.
OperationMode
Core::get_operation_mode_regular()
{
    /* operation_mode_regular is the same as operation_mode unless there's an 
    override in place, in which case operation_mode is the current override mode and 
    operation_mode_regular is the mode that will be restored once all overrides are removed
    */
    return operation_mode_regular;
}

//! Checks if operation_mode is an override.
bool
Core::is_operation_mode_an_override()
{
    return !!operation_mode_overrides.size();
}

//! Sets the operation mode.
void
Core::set_operation_mode(OperationMode mode)
{
    set_operation_mode_internal( mode, true );
}

//! Temporarily overrides the operation mode.
void
Core::set_operation_mode_override( OperationMode mode, const std::string &id )
{
    if( !id.size() )
        return;

    set_operation_mode_internal( mode, false, id );
}

//! Removes the overriden operation mode.
void
Core::remove_operation_mode_override( const std::string &id )
{
    TRACE_ENTER( "Core::remove_operation_mode_override" );

    if( !id.size() || !operation_mode_overrides.count( id ) )
        return;

    operation_mode_overrides.erase( id );

    /* If there are other overrides still in the queue then pass in the first 
    override in the map. set_operation_mode_internal() will then search the 
    map for the most important override and set it as the active operation mode.
    */
    if( operation_mode_overrides.size() )
    {
        set_operation_mode_internal( 
            operation_mode_overrides.begin()->second, 
            false, 
            operation_mode_overrides.begin()->first
            );
    }
    else
    {
        /* if operation_mode_regular is the same as the active operation mode then just 
        signal the mode has changed. During overrides the signal is not sent so it needs to 
        be sent now. Because the modes are the same it would not be called by 
        set_operation_mode_internal().
        */
        if( operation_mode_regular == operation_mode )
        {
            TRACE_MSG( "Only calling core_event_operation_mode_changed()." );
            if( core_event_listener )
                core_event_listener->core_event_operation_mode_changed( operation_mode_regular );
        }
        else
            set_operation_mode_internal( operation_mode_regular, false );
    }

    TRACE_EXIT();
}

//! Set the operation mode.
void
Core::set_operation_mode_internal(
    OperationMode mode, 
    bool persistent, 
    const std::string &override_id /* default param: empty string */
    )
{
  TRACE_ENTER_MSG("Core::set_operation_mode", ( persistent ? "persistent" : "" ) );

  if( override_id.size() )
  {
      TRACE_MSG( "override_id: " << override_id );
  }

  TRACE_MSG( "Incoming/requested mode is "
      << ( mode == OPERATION_MODE_NORMAL ? "OPERATION_MODE_NORMAL" :
            mode == OPERATION_MODE_SUSPENDED ? "OPERATION_MODE_SUSPENDED" :
                mode == OPERATION_MODE_QUIET ? "OPERATION_MODE_QUIET" : "???" )
      << ( override_id.size() ? " (override)" : " (regular)" )
      );

  TRACE_MSG( "Current mode is "
      << ( mode == OPERATION_MODE_NORMAL ? "OPERATION_MODE_NORMAL" :
            mode == OPERATION_MODE_SUSPENDED ? "OPERATION_MODE_SUSPENDED" :
                mode == OPERATION_MODE_QUIET ? "OPERATION_MODE_QUIET" : "???" )
      << ( operation_mode_overrides.size() ? " (override)" : " (regular)" )
      );
  
  if( ( mode != OPERATION_MODE_NORMAL )
      && ( mode != OPERATION_MODE_QUIET )
      && ( mode != OPERATION_MODE_SUSPENDED )
      )
  {
      TRACE_RETURN( "No change: incoming invalid" );
      return;
  }

  /* If the incoming operation mode is regular and the current operation mode is an 
  override then save the incoming operation mode and return.
  */
  if( !override_id.size() && operation_mode_overrides.size() )
  {
      operation_mode_regular = mode;

      int cm;
      if( persistent 
          && ( !get_configurator()->get_value( CoreConfig::CFG_KEY_OPERATION_MODE, cm ) 
              || ( cm != mode ) )
          )
          get_configurator()->set_value( CoreConfig::CFG_KEY_OPERATION_MODE, mode );

      TRACE_RETURN( "No change: current is an override type but incoming is regular" );
      return;
  }
  
  // If the incoming operation mode is tagged as an override
  if( override_id.size() )
  {
      // Add this override to the map
      operation_mode_overrides[ override_id ] = mode;

      /* Find the most important override. Override modes in order of importance:
      OPERATION_MODE_SUSPENDED, OPERATION_MODE_QUIET, OPERATION_MODE_NORMAL
      */
      for( map<std::string, OperationMode>::iterator i = operation_mode_overrides.begin();
          ( i != operation_mode_overrides.end() );
          ++i
          )
      {
          if( i->second == OPERATION_MODE_SUSPENDED )
          {
              mode = OPERATION_MODE_SUSPENDED;
              break;
          }

          if( ( i->second == OPERATION_MODE_QUIET )
              && ( mode == OPERATION_MODE_NORMAL )
              )
          {
              mode = OPERATION_MODE_QUIET;
          }
      }
  }


  if (operation_mode != mode)
  {
      TRACE_MSG( "Changing active operation mode to "
          << ( mode == OPERATION_MODE_NORMAL ? "OPERATION_MODE_NORMAL" :
                mode == OPERATION_MODE_SUSPENDED ? "OPERATION_MODE_SUSPENDED" :
                    mode == OPERATION_MODE_QUIET ? "OPERATION_MODE_QUIET" : "???" )
          );

      OperationMode previous_mode = operation_mode;

      operation_mode = mode;

      if( !operation_mode_overrides.size() )
          operation_mode_regular = operation_mode;

      if (operation_mode == OPERATION_MODE_SUSPENDED)
      {
          TRACE_MSG("Force idle");
          force_idle();
          monitor->suspend();
          stop_all_breaks();

          for (int i = 0; i < BREAK_ID_SIZEOF; ++i)
          {
              if (breaks[i].is_enabled())
              {
                  breaks[i].get_timer()->set_insensitive_mode(INSENSITIVE_MODE_IDLE_ALWAYS);
              }
          }
      }
      else if (previous_mode == OPERATION_MODE_SUSPENDED)
      {
          // stop_all_breaks again will reset insensitive mode (that is good)
          stop_all_breaks();
          monitor->resume();
      }

      if (operation_mode == OPERATION_MODE_QUIET)
      {
          stop_all_breaks();
      }

      if( !operation_mode_overrides.size() )
      {
          /* The two functions in this block will trigger signals that can call back into this function.
          Only if there are no overrides in place will that reentrancy be ok from here.
          Otherwise the regular/user mode to restore would be overwritten.
          */

          if( persistent )
              get_configurator()->set_value( CoreConfig::CFG_KEY_OPERATION_MODE, operation_mode );

          if( core_event_listener )
              core_event_listener->core_event_operation_mode_changed( operation_mode );
      }
  }

  TRACE_EXIT();
}

//! Retrieves the usage mode.
UsageMode
Core::get_usage_mode()
{
  return usage_mode;
}


//! Sets the usage mode.
void
Core::set_usage_mode(UsageMode mode)
{
  set_usage_mode_internal(mode, true);
}

//! Sets the usage mode.
void
Core::set_usage_mode_internal(UsageMode mode, bool persistent)
{
  if (usage_mode != mode)
    {
      usage_mode = mode;

      for (int i = 0; i < BREAK_ID_SIZEOF; i++)
        {
          breaks[i].set_usage_mode(mode);
        }

      if (persistent)
        {
          get_configurator()->set_value(CoreConfig::CFG_KEY_USAGE_MODE, mode);
        }

      if (core_event_listener != NULL)
        {
          core_event_listener->core_event_usage_mode_changed(mode);
        }
    }
}

//! Sets the listener for core events.
void
Core::set_core_events_listener(ICoreEventListener *l)
{
  core_event_listener = l;
}


//! Forces the start of the specified break.
void
Core::force_break(BreakId id, BreakHint break_hint)
{
  do_force_break(id, break_hint);

#ifdef HAVE_BROKEN_DISTRIBUTION
  BreakLinkEvent event(id, ((break_hint & BREAK_HINT_USER_INITIATED) != 0) ?
                       BreakLinkEvent::BREAK_EVENT_USER_FORCE_BREAK :
                       BreakLinkEvent::BREAK_EVENT_SYST_FORCE_BREAK) ;
  network->send_event(&event);
#endif
}


//! Forces the start of the specified break.
void
Core::do_force_break(BreakId id, BreakHint break_hint)
{
  TRACE_ENTER_MSG("Core::do_force_break", id);
  BreakControl *microbreak_control = breaks[BREAK_ID_MICRO_BREAK].get_break_control();
  BreakControl *breaker = breaks[id].get_break_control();

  if (id == BREAK_ID_REST_BREAK &&
      (microbreak_control->get_break_state() == BreakControl::BREAK_ACTIVE))
    {
      microbreak_control->stop_break(false);
      resume_break = BREAK_ID_MICRO_BREAK;
      TRACE_MSG("Resuming Micro break");
    }

  breaker->force_start_break(break_hint);
  TRACE_EXIT();
}


//! Announces a change in time.
void
Core::time_changed()
{
  TRACE_ENTER("Core::time_changed");

  // In case out timezone changed..
  tzset();

  // A change of system time idle handled by process_timewarp.
  // This is used to handle a change in timezone on windows.
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      breaks[i].get_timer()->shift_time(0);
    }

  TRACE_EXIT();
}


//! Announces a powersave state.
void
Core::set_powersave(bool down)
{
  TRACE_ENTER_MSG("Core::set_powersave", down);
  TRACE_MSG(powersave << " " << powersave_resume_time << " " << operation_mode);

  if (down)
    {
      if (!powersave)
        {
          // Computer is going down
          set_operation_mode_override( OPERATION_MODE_SUSPENDED, "powersave" );
          powersave_resume_time = 0;
          powersave = true;
        }
      
      save_state();
      statistics->update();
    }
  else
    {
      // Computer is coming back
      // leave powersave true until the timewarp is detected
      // or until some time has passed
      if (powersave_resume_time == 0)
        {
          powersave_resume_time = current_time ? current_time : 1;
          TRACE_MSG("set resume time " << powersave_resume_time);
        }

      TRACE_MSG("resume time " << powersave_resume_time);
      remove_operation_mode_override( "powersave" );
    }
  TRACE_EXIT();
}


//! Sets the insist policy.
/*!
 *  The insist policy determines what to do when the user is active while
 *  taking a break.
 */
void
Core::set_insist_policy(ICore::InsistPolicy p)
{
  TRACE_ENTER_MSG("Core::set_insist_policy", p);

  if (active_insist_policy != ICore::INSIST_POLICY_INVALID &&
      insist_policy != p)
    {
      TRACE_MSG("refreeze " << active_insist_policy);
      defrost();
      insist_policy = p;
      freeze();
    }
  else
    {
      insist_policy = p;
    }
  TRACE_EXIT();
}


//! Gets the insist policy.
ICore::InsistPolicy
Core::get_insist_policy() const
{
  return insist_policy;
}



// ! Forces all monitors to be idle.
void
Core::force_idle()
{
  TRACE_ENTER("Core::force_idle");
  force_idle(BREAK_ID_NONE);
  TRACE_EXIT();
}


void
Core::force_idle(BreakId break_id)
{
  TRACE_ENTER("Core::force_idle_for_break");
  
  monitor->force_idle();
  
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      if (break_id == BREAK_ID_NONE || i == break_id)
        {
          IActivityMonitor *am = breaks[i].get_timer()->get_activity_monitor();
          if (am != NULL)
            {
              am->force_idle();
            }
        }
      
      breaks[i].get_timer()->force_idle();
    }
  TRACE_EXIT();
}


/********************************************************************************/
/**** Break Response                                                       ******/
/********************************************************************************/

//! User postpones the specified break.
void
Core::postpone_break(BreakId break_id)
{
  do_postpone_break(break_id);

#ifdef HAVE_BROKEN_DISTRIBUTION
  BreakLinkEvent event(break_id, BreakLinkEvent::BREAK_EVENT_USER_POSTPONE);
  network->send_event(&event);
#endif
}


//! User skips the specified break.
void
Core::skip_break(BreakId break_id)
{
  do_skip_break(break_id);

#ifdef HAVE_BROKEN_DISTRIBUTION
  BreakLinkEvent event(break_id, BreakLinkEvent::BREAK_EVENT_USER_SKIP);
  network->send_event(&event);
#endif
}


//! User stops the prelude.
void
Core::stop_prelude(BreakId break_id)
{
  TRACE_ENTER_MSG("Core::stop_prelude", break_id);
  do_stop_prelude(break_id);

#ifdef HAVE_BROKEN_DISTRIBUTION
  BreakLinkEvent event(break_id, BreakLinkEvent::BREAK_EVENT_SYST_STOP_PRELUDE);
  network->send_event(&event);
#endif

  TRACE_EXIT();
}


//! User postpones the specified break.
void
Core::do_postpone_break(BreakId break_id)
{
  if (break_id >= 0 && break_id < BREAK_ID_SIZEOF)
    {
      BreakControl *bc = breaks[break_id].get_break_control();
      bc->postpone_break();
    }
}


//! User skips the specified break.
void
Core::do_skip_break(BreakId break_id)
{
  if (break_id >= 0 && break_id < BREAK_ID_SIZEOF)
    {
      BreakControl *bc = breaks[break_id].get_break_control();
      bc->skip_break();
    }
}


//!
void
Core::do_stop_prelude(BreakId break_id)
{
  TRACE_ENTER_MSG("Core::do_stop_prelude", break_id);
  if (break_id >= 0 && break_id < BREAK_ID_SIZEOF)
    {
      BreakControl *bc = breaks[break_id].get_break_control();
      bc->stop_prelude();
    }
  TRACE_EXIT();
}


/********************************************************************************/
/**** Break handling                                                       ******/
/********************************************************************************/

//! Periodic heartbeat.
void
Core::heartbeat()
{
  TRACE_ENTER("Core::heartbeat");
  assert(application != NULL);

#ifdef HAVE_TESTS
  if (!manual_clock)
    {
      current_time = time(NULL);
    }
#else
  // Set current time.
  current_time = time(NULL);
#endif
  TRACE_MSG("Time = " << current_time);

  // Performs timewarp checking.
  bool warped = process_timewarp();

  // Process configuration
  configurator->heartbeat();

  if (!warped)
    {
      // Perform state computation.
      process_state();
    }

  // Perform timer processing.
  process_timers();

  // Send heartbeats to other components.
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      BreakControl *bc = breaks[i].get_break_control();
      if (bc != NULL && bc->need_heartbeat())
        {
          bc->heartbeat();
        }
    }

#ifdef HAVE_BROKEN_DISTRIBUTION
  network->heartbeat();
#endif

  // Make state persistent.
  if (current_time % SAVESTATETIME == 0)
    {
      statistics->update();
      save_state();
    }

  // Done.
  last_process_time = current_time;

  TRACE_EXIT();
}



//! Computes the current state.
void
Core::process_state()
{
  // Default
  local_state = monitor->get_current_state();

  map<std::string, time_t>::iterator i = external_activity.begin();
  while (i != external_activity.end())
    {
      map<std::string, time_t>::iterator next = i;
      next++;

      if (i->second >= current_time)
        {
          local_state = ACTIVITY_ACTIVE;
        }
      else
        {
          external_activity.erase(i);
        }

      i = next;
    }

  monitor_state = local_state;

#ifdef HAVE_TESTS
  if (fake_monitor != NULL)
    {
      monitor_state = fake_monitor->get_current_state();
    }
#endif
}


void
Core::report_external_activity(std::string who, bool act)
{
  TRACE_ENTER_MSG("Core::report_external_activity", who << " " << act);
  if (act)
    {
      external_activity[who] = current_time + 10;
    }
  else
    {
      external_activity.erase(who);
    }
  TRACE_EXIT();
}


void
Core::is_timer_running(BreakId id, bool &value)
{
  Timer *timer = get_timer(id);
  value = timer->get_state() == STATE_RUNNING;
}


void
Core::get_timer_idle(BreakId id, int *value)
{
  Timer *timer = get_timer(id);
  *value = (int) timer->get_elapsed_idle_time();
}


void
Core::get_timer_elapsed(BreakId id, int *value)
{
  Timer *timer = get_timer(id);
  *value = (int) timer->get_elapsed_time();
}


void
Core::get_timer_overdue(BreakId id, int *value)
{
  Timer *timer = get_timer(id);
  *value = (int) timer->get_total_overdue_time();
}

//! Processes all timers.
void
Core::process_timers()
{
  TRACE_ENTER("Core::process_timers");

  TimerInfo infos[BREAK_ID_SIZEOF];

  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      Timer *timer = breaks[i].get_timer();
      
      infos[i].enabled = breaks[i].is_enabled();
      if (infos[i].enabled)
        {
          timer->enable();
          if (i == BREAK_ID_DAILY_LIMIT)
            {
              timer->set_limit_enabled(timer->get_limit() > 0);
            }
        }
      else
        {
          if (i != BREAK_ID_DAILY_LIMIT)
            {
              timer->disable();
            }
          else
            {
              timer->set_limit_enabled(false);
            }
        }

      // First process only timer that do not have their
      // own activity monitor.
      if (!(timer->has_activity_monitor()))
        {
          timer->process(monitor_state, infos[i]);
        }
    }

  // And process timer with activity monitor.
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      if (breaks[i].get_timer()->has_activity_monitor())
        {
          breaks[i].get_timer()->process(monitor_state, infos[i]);
        }

#ifdef HAVE_BROKEN_DISTRIBUTION
      TimerState timer_state = breaks[i].get_timer()->get_state();
      TRACE_MSG("break" << i << " time" << current_time << " timer state " << timer_state);
      network->report_timer_state(i, timer_state == STATE_RUNNING);
#endif
    }


  // Process all timer events.
  for (int i = BREAK_ID_SIZEOF - 1; i >= 0;  i--)
    {
      TimerInfo &info = infos[i];
      if (breaks[i].is_enabled())
        {
          timer_action((BreakId)i, info);
        }

      if (i == BREAK_ID_DAILY_LIMIT &&
          (info.event == TIMER_EVENT_NATURAL_RESET ||
           info.event == TIMER_EVENT_RESET))
        {
          statistics->set_counter(Statistics::STATS_VALUE_TOTAL_ACTIVE_TIME, (int) info.elapsed_time);
          statistics->start_new_day();

          daily_reset();
        }
    }

  TRACE_EXIT();
}

#if defined(PLATFORM_OS_WIN32)

//! Process a possible timewarp on Win32
bool
Core::process_timewarp()
{
  bool ret = false;

  TRACE_ENTER("Core::process_timewarp");
#ifdef HAVE_TESTS
  if (manual_clock)
    {
      // Don't to warping is manual clock mode
      TRACE_EXIT();
      return;
    }
#endif

  if (last_process_time != 0)
    {
      time_t gap = current_time - 1 - last_process_time;
  
      if (abs((int)gap) > 5)
        {
          TRACE_MSG("gap " << gap << " " << powersave << " " << operation_mode << " " << powersave_resume_time << " " << current_time);

          if (!powersave)
            {
              TRACE_MSG("Time warp of " << gap << " seconds. Correcting");

              force_idle();

              monitor->shift_time((int)gap);
              for (int i = 0; i < BREAK_ID_SIZEOF; i++)
                {
                  breaks[i].get_timer()->shift_time((int)gap);
                }

              monitor_state = ACTIVITY_IDLE;
              ret = true;
            }
          else
            {
              TRACE_MSG("Time warp of " << gap << " seconds because of powersave");

              // In case the windows message was lost. some people reported that
              // workrave never restarted the timers...
              remove_operation_mode_override( "powersave" );
            }
        }
      
      if (powersave && powersave_resume_time != 0 && current_time > powersave_resume_time + 30)
        {
          TRACE_MSG("End of time warp after powersave");

          powersave = false;
          powersave_resume_time = 0;
        }
    }
  TRACE_EXIT();
  return ret;
}

#else

//! Process a possible timewarp On Non-Windows
bool
Core::process_timewarp()
{
  bool ret = false;

  TRACE_ENTER("Core::process_timewarp");
  if (last_process_time != 0)
    {
      int gap = current_time - 1 - last_process_time;

      if (gap >= 30)
        {
          TRACE_MSG("Time warp of " << gap << " seconds. Powersafe");

          force_idle();

          int save_current_time = current_time;

          current_time = last_process_time + 1;
          monitor_state = ACTIVITY_IDLE;

          process_timers();

          current_time = save_current_time;
          ret = true;
        }
    }

  TRACE_EXIT();
  return ret;
}

#endif

//! Notication of a timer action.
/*!
 *  \param timerId ID of the timer that caused the action.
 *  \param action action that is performed by the timer.
*/
void
Core::timer_action(BreakId id, TimerInfo info)
{
  // No breaks when mode is quiet,
  if (operation_mode == OPERATION_MODE_QUIET &&
      info.event == TIMER_EVENT_LIMIT_REACHED)
    {
      return;
    }

  BreakControl *breaker = breaks[id].get_break_control();
  Timer *timer = breaks[id].get_timer();

  assert(breaker != NULL && timer != NULL);

  switch (info.event)
    {
    case TIMER_EVENT_LIMIT_REACHED:
      if (breaker->get_break_state() == BreakControl::BREAK_INACTIVE)
        {
          start_break(id);
        }
      break;

    case TIMER_EVENT_NATURAL_RESET:
      statistics->increment_break_counter(id, Statistics::STATS_BREAKVALUE_NATURAL_TAKEN);
      // FALLTHROUGH

    case TIMER_EVENT_RESET:
      if (breaker->get_break_state() == BreakControl::BREAK_ACTIVE)
        {
          breaker->stop_break();
        }
      break;

    default:
      break;
    }
}


//! starts the specified break.
/*!
 *  \param break_id ID of the timer that caused the break.
 */
void
Core::start_break(BreakId break_id, BreakId resume_this_break)
{
  // Don't show MB when RB is active, RB when DL is active.
  for (int bi = break_id; bi <= BREAK_ID_DAILY_LIMIT; bi++)
    {
      if (breaks[bi].get_break_control()->get_break_state() == BreakControl::BREAK_ACTIVE)
        {
          return;
        }
    }

  // Advance restbreak if it follows within 30s after the end of a microbreak
  if (break_id == BREAK_ID_REST_BREAK && resume_this_break == BREAK_ID_NONE)
    {
      breaks[BREAK_ID_REST_BREAK].override(BREAK_ID_REST_BREAK);
    }

  if (break_id == BREAK_ID_MICRO_BREAK && breaks[BREAK_ID_REST_BREAK].is_enabled())
    {
      Timer *rb_timer = breaks[BREAK_ID_REST_BREAK].get_timer();
      assert(rb_timer != NULL);

      bool activity_sensitive = breaks[BREAK_ID_REST_BREAK].get_timer_activity_sensitive();

      // Only advance when
      // 0. It is activity sensitive
      // 1. we have a next limit reached time.
      if (activity_sensitive &&
          rb_timer->get_next_limit_time() > 0)
        {
          Timer *timer = breaks[break_id].get_timer();

          time_t duration = timer->get_auto_reset();
          time_t now = get_time();

          if (now + duration + 30 >= rb_timer->get_next_limit_time())
            {
              breaks[BREAK_ID_REST_BREAK].override(BREAK_ID_MICRO_BREAK);

              start_break(BREAK_ID_REST_BREAK, BREAK_ID_MICRO_BREAK);

              // Snooze timer before the limit was reached. Just to make sure
              // that it doesn't reach its limit again when elapsed == limit
              rb_timer->snooze_timer();
              return;
            }
        }
    }

  // Stop microbreak when a restbreak starts. should not happend.
  // restbreak should be advanced.
  for (int bi = BREAK_ID_MICRO_BREAK; bi < break_id; bi++)
    {
      if (breaks[bi].get_break_control()->get_break_state() == BreakControl::BREAK_ACTIVE)
        {
          breaks[bi].get_break_control()->stop_break(false);
        }
    }

  // If break 'break_id' ends, and break 'resume_this_break' is still
  // active, resume it...
  resume_break = resume_this_break;

  BreakControl *breaker = breaks[break_id].get_break_control();
  breaker->start_break();
}


//! Sets the freeze state of all breaks.
void
Core::set_freeze_all_breaks(bool freeze)
{
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      Timer *t = breaks[i].get_timer();
      assert(t != NULL);
      if (!t->has_activity_monitor())
        {
          t->freeze_timer(freeze);
        }
    }
}


//! Stops all breaks.
void
Core::stop_all_breaks()
{
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      BreakControl *bc = breaks[i].get_break_control();
      assert(bc != NULL);
      bc->stop_break(false);
    }
}


/********************************************************************************/
/**** Misc                                                                 ******/
/********************************************************************************/

//! Performs a reset when the daily limit is reached.
void
Core::daily_reset()
{
  TRACE_ENTER("Core::daily_reset");
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      Timer *t = breaks[i].get_timer();
      assert(t != NULL);

      time_t overdue = t->get_total_overdue_time();

      statistics->set_break_counter(((BreakId)i),
                                    Statistics::STATS_BREAKVALUE_TOTAL_OVERDUE, (int)overdue);

      t->daily_reset_timer();
    }

  save_state();

  TRACE_EXIT();
}


//! Saves the current state.
void
Core::save_state() const
{
  stringstream ss;
  ss << Util::get_home_directory();
  ss << "state" << ends;

  ofstream stateFile(ss.str().c_str());

  stateFile << "WorkRaveState 3"  << endl
            << get_time() << endl;

  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      string stateStr = breaks[i].get_timer()->serialize_state();

      stateFile << stateStr << endl;
    }

  stateFile.close();
}


//! Loads miscellaneous
void
Core::load_misc()
{
  configurator->rename_key("gui/operation-mode", CoreConfig::CFG_KEY_OPERATION_MODE);
  configurator->add_listener(CoreConfig::CFG_KEY_OPERATION_MODE, this);
  configurator->add_listener(CoreConfig::CFG_KEY_USAGE_MODE, this);

  int mode;
  if (! get_configurator()->get_value(CoreConfig::CFG_KEY_OPERATION_MODE, mode))
    {
      mode = OPERATION_MODE_NORMAL;
    }
  set_operation_mode(OperationMode(mode));

  if (! get_configurator()->get_value(CoreConfig::CFG_KEY_USAGE_MODE, mode))
    {
      mode = USAGE_MODE_NORMAL;
    }
  set_usage_mode(UsageMode(mode));
}


//! Loads the current state.
void
Core::load_state()
{
  stringstream ss;
  ss << Util::get_home_directory();
  ss << "state" << ends;

  ifstream stateFile(ss.str().c_str());

  int version = 0;
  bool ok = stateFile.good();

  if (ok)
    {
      string tag;
      stateFile >> tag;

      ok = (tag == WORKRAVESTATE);
    }

  if (ok)
    {
      stateFile >> version;

      ok = (version >= 1 && version <= 3);
    }

  if (ok)
    {
      time_t saveTime;
      stateFile >> saveTime;
    }

  while (ok && !stateFile.eof())
    {
      string id;
      stateFile >> id;

      for (int i = 0; i < BREAK_ID_SIZEOF; i++)
        {
          if (breaks[i].get_timer()->get_id() == id)
            {
              string state;
              getline(stateFile, state);

              breaks[i].get_timer()->deserialize_state(state, version);
              break;
            }
        }
    }
}


//! Post an event.
void
Core::post_event(CoreEvent event)
{
  if (core_event_listener != NULL)
    {
      core_event_listener->core_event_notify(event);
    }
}


//! Excecute the insist policy.
void
Core::freeze()
{
  TRACE_ENTER_MSG("Core::freeze", insist_policy);
  ICore::InsistPolicy policy = insist_policy;

  switch (policy)
    {
    case ICore::INSIST_POLICY_IGNORE:
      {
        // Ignore all activity during break by suspending the activity monitor.
        monitor->suspend();
      }
      break;
    case ICore::INSIST_POLICY_HALT:
      {
        // Halt timer when the user is active.
        set_freeze_all_breaks(true);
      }
      break;
    case ICore::INSIST_POLICY_RESET:
      // reset the timer when the user becomes active.
      // default.
      break;

    default:
      break;
    }

  active_insist_policy = policy;
  TRACE_EXIT();
}


//! Undo the insist policy.
void
Core::defrost()
{
  TRACE_ENTER_MSG("Core::defrost", active_insist_policy);

  switch (active_insist_policy)
    {
    case ICore::INSIST_POLICY_IGNORE:
      {
        // Resumes the activity monitor, if not suspended.
        if (operation_mode != OPERATION_MODE_SUSPENDED)
          {
            monitor->resume();
          }
      }
      break;
    case ICore::INSIST_POLICY_HALT:
      {
        // Desfrost timers.
        set_freeze_all_breaks(false);
      }
      break;

    default:
      break;
    }

  active_insist_policy = ICore::INSIST_POLICY_INVALID;
  TRACE_EXIT();
}


//! Is the user currently active?
bool
Core::is_user_active() const
{
  return monitor_state == ACTIVITY_ACTIVE;
}


/********************************************************************************/
/**** Networking support                                                   ******/
/********************************************************************************/

#ifdef HAVE_BROKEN_DISTRIBUTION
//! Process event from remote Workrave.
void
Core::event_received(LinkEvent *event)
{
  TRACE_ENTER_MSG("Core::event_received", event->str());

  const BreakLinkEvent *ble = dynamic_cast<const BreakLinkEvent *>(event);
  if (ble != NULL)
    {
      break_event_received(ble);
    }

  const CoreLinkEvent *cle = dynamic_cast<const CoreLinkEvent *>(event);
  if (cle != NULL)
    {
      core_event_received(cle);
    }

  const TimerStateLinkEvent *tsle = dynamic_cast<const TimerStateLinkEvent *>(event);
  if (tsle != NULL)
    {
      timer_state_event_received(tsle);
    }
  
  TRACE_EXIT();
}


//! Process Break event
void
Core::break_event_received(const BreakLinkEvent *event)
{
  TRACE_ENTER_MSG("Core::break_event_received", event->str());

  BreakId break_id = event->get_break_id();
  BreakLinkEvent::BreakEvent break_event = event->get_break_event();

  switch(break_event)
    {
    case BreakLinkEvent::BREAK_EVENT_USER_POSTPONE:
      do_postpone_break(break_id);
      break;

    case BreakLinkEvent::BREAK_EVENT_USER_SKIP:
      do_skip_break(break_id);
      break;

    case BreakLinkEvent::BREAK_EVENT_USER_FORCE_BREAK:
      do_force_break(break_id, BREAK_HINT_USER_INITIATED);
      break;

    case BreakLinkEvent::BREAK_EVENT_SYST_FORCE_BREAK:
      do_force_break(break_id, (BreakHint)0);
      break;

    case BreakLinkEvent::BREAK_EVENT_SYST_STOP_PRELUDE:
      do_stop_prelude(break_id);
      break;

    default:
      break;
    }

  TRACE_EXIT();
}


//! Process Core event
void
Core::core_event_received(const CoreLinkEvent *event)
{
  TRACE_ENTER_MSG("Core::core_event_received", event->str());

  CoreLinkEvent::CoreEvent core_event = event->get_core_event();

  switch(core_event)
    {
    case CoreLinkEvent::CORE_EVENT_MODE_SUSPENDED:
      set_operation_mode_no_event(OPERATION_MODE_SUSPENDED);
      break;

    case CoreLinkEvent::CORE_EVENT_MODE_QUIET:
      set_operation_mode_no_event(OPERATION_MODE_QUIET);
      break;

    case CoreLinkEvent::CORE_EVENT_MODE_NORMAL:
      set_operation_mode_no_event(OPERATION_MODE_NORMAL);
      break;

    default:
      break;
    }

  TRACE_EXIT();
}


//! Process Timer state event
void
Core::timer_state_event_received(const TimerStateLinkEvent *event)
{
  TRACE_ENTER_MSG("Core::timer_state_event_received", event->str());

  const std::vector<int> &idle_times = event->get_idle_times();
  const std::vector<int> &active_times = event->get_active_times();
    
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      Timer *timer = breaks[i].get_timer();

      time_t old_idle = timer->get_elapsed_idle_time();
      time_t old_active = timer->get_elapsed_time();

      int idle = idle_times[i];
      int active = active_times[i];

      TRACE_MSG("Timer " << i <<
                " idle " <<  idle << " " << old_idle <<
                " active" << active << " " << old_active); 

      time_t remote_active_since = 0;
      bool remote_active = network->is_remote_active(event->get_source(),
                                                        remote_active_since);
      
      if (abs((int)(idle - old_idle)) >= 2 ||
          abs((int)(active - old_active)) >= 2 ||
          /* Remote party is active, and became active after us */
          (remote_active && remote_active_since > active_since))
        {
          timer->set_state(active, idle);
        }
    }
 
  TRACE_EXIT();
}


//! Broadcast current timer state.
void
Core::broadcast_state()
{
  std::vector<int> idle_times;
  std::vector<int> active_times;
  
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      Timer *timer = breaks[i].get_timer();

      idle_times.push_back((int)timer->get_elapsed_idle_time());
      active_times.push_back((int)timer->get_elapsed_time());
    }

  TimerStateLinkEvent event(idle_times, active_times);
  network->send_event(&event);
}

#endif

namespace workrave
{
  std::string operator%(const string &key, BreakId id)
  {
    IBreak *b = Core::get_instance()->get_break(id);

    string str = key;
    string::size_type pos = 0;
    string name = b->get_name();

    while ((pos = str.find("%b", pos)) != string::npos)
      {
        str.replace(pos, 2, name);
        pos++;
      }

    return str;
  }
}

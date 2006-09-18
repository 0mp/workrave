// Break.cc
//
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 Rob Caelers & Raymond Penners
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

#include "debug.hh"
#include "nls.h"

#include "Configurator.hh"
#include "Core.hh"
#include "BreakControl.hh"
#include "Timer.hh"
#include "Break.hh"
#include "TimerActivityMonitor.hh"

const string Break::CFG_KEY_TIMER_PREFIX = "timers/";

const string Break::CFG_KEY_TIMER_LIMIT = "/limit";
const string Break::CFG_KEY_TIMER_AUTO_RESET = "/auto_reset";
const string Break::CFG_KEY_TIMER_RESET_PRED = "/reset_pred";
const string Break::CFG_KEY_TIMER_SNOOZE = "/snooze";
const string Break::CFG_KEY_TIMER_MONITOR = "/monitor";
const string Break::CFG_KEY_TIMER_ACTIVITY_SENSITIVE = "/activity_sensitive";

const string Break::CFG_KEY_BREAK_PREFIX = "gui/breaks/";

const string Break::CFG_KEY_BREAK_MAX_PRELUDES = "/max_preludes";
const string Break::CFG_KEY_BREAK_MAX_POSTPONE = "/max_postpone";
const string Break::CFG_KEY_BREAK_IGNORABLE = "/ignorable_break";
const string Break::CFG_KEY_BREAK_ENABLED = "/enabled";
const string Break::CFG_KEY_BREAK_EXERCISES = "/exercises";

struct Defaults
{
  string name;
  
  // Timer settings.
  int limit;
  int auto_reset;
  string resetpred;
  int snooze;

  // Break settings
  int max_preludes;
  int max_postpone;
  bool ignorable_break;
  int exercises;

} default_config[] =
  {
    {
      // FIXME: Rename to micro_break, but in a backwards compatible manner.
      "micro_pause",
      3*60, 30, "", 150,
      3, -1, true,  
      0,
    },

    {
      "rest_break",
      45*60, 10*60, "", 180,
      3, -1, true,
      3
    },
    
    {
      "daily_limit",
      14400, 0, "day/4:00", 20 * 60,
      3, -1, true,
      0
    }
  };



//! Constucts a new Break
Break::Break() :
  break_id(BREAK_ID_NONE),
  configurator(NULL),
  application(NULL),
  timer(NULL),
  break_control(NULL),
  enabled(true)
{
  TRACE_ENTER("Break:Break");
  TRACE_EXIT()
}


//! Initializes the break.
void
Break::init(BreakId id, AppInterface *app)
{
  TRACE_ENTER("Break::init");

  break_id = id;
  Core *core = Core::get_instance();
  configurator = core->get_configurator();
  application = app;

  Defaults &def = default_config[break_id];

  break_name = def.name;
  timer = new Timer(core);
  timer->set_id(break_name);
  break_control = new BreakControl(break_id, core, app, timer);

  init_timer();
  init_break_control();

  TRACE_EXIT()
}

//! Destructor.
Break::~Break()
{
  TRACE_ENTER("Break:~Break");
  delete break_control;
  delete timer;
  TRACE_EXIT();
}


//! Returns whether the break is enabled.
bool
Break::is_enabled() const
{
  return enabled;
}



//! Returns the name of the break (used in configuration)
string
Break::get_name() const
{
  return break_name;
}


//! Returns the timer.
Timer *
Break::get_timer() const
{
  return timer;
}


//! Returns the Break controller.
BreakControl *
Break::get_break_control()
{
  return break_control;
}


// Initialize the timer based.
void
Break::init_timer()
{
  timer_prefix = CFG_KEY_TIMER_PREFIX + break_name;

  update_timer_config();
  load_timer_config();
  load_timer_monitor_config();
  
  timer->enable();
  timer->stop_timer();
  
  configurator->add_listener(CFG_KEY_TIMER_PREFIX + break_name, this);
}


void
Break::update_timer_config()
{
  set_timer_limit(get_timer_limit());
  set_timer_auto_reset(get_timer_auto_reset());
  set_timer_reset_pred(get_timer_reset_pred());
  set_timer_snooze(get_timer_snooze());
  set_timer_activity_sensitive(get_timer_activity_sensitive());
  
}

//! Load the configuration of the timer.
void
Break::load_timer_config()
{
  // Read break limit.
  int limit = get_timer_limit();;
  timer->set_limit(limit);
  timer->set_limit_enabled(limit > 0);

  // Read autoreset interval
  int autoreset = get_timer_auto_reset();;
  timer->set_auto_reset(autoreset);
  timer->set_auto_reset_enabled(autoreset > 0);

  // Read reset predicate
  string reset_pred = get_timer_reset_pred();
  if (reset_pred != "")
    {
      timer->set_auto_reset(reset_pred);
    }

  // Read the snooze time.
  int snooze = get_timer_snooze();
  timer->set_snooze_interval(snooze);

  // Read the activity insensitive flag
  bool sensitive = get_timer_activity_sensitive();
  timer->set_activity_sensitive(sensitive);
}


//! Load the monitor setting for the timer.
void
Break::load_timer_monitor_config()
{
  string monitor_name;

  bool ret = configurator->get_value(timer_prefix + CFG_KEY_TIMER_MONITOR, &monitor_name);
  if (ret && monitor_name != "")
    {
      Core *core = Core::get_instance();
      Timer *master = core->get_timer(monitor_name);
      if (master != NULL)
        {
          TimerActivityMonitor *am = new TimerActivityMonitor(master);
          timer->set_activity_monitor(am);
        }
    }
  else
    {
      timer->set_activity_monitor(NULL);
    }
}


// Initialize the break control.
void
Break::init_break_control()
{
  break_prefix = CFG_KEY_BREAK_PREFIX + break_name;

  update_break_config();
  load_break_control_config();
  configurator->add_listener(CFG_KEY_BREAK_PREFIX + break_name, this);
}


void
Break::update_break_config()
{
  set_break_max_preludes(get_break_max_preludes());
  set_break_max_postpone(get_break_max_postpone());
  set_break_ignorable(get_break_ignorable());
  set_break_enabled(get_break_enabled());
  set_break_exercises(get_break_exercises());
}


void
Break::load_break_control_config()
{
  break_control->set_max_preludes(get_break_max_preludes());
  break_control->set_max_postpone(get_break_max_postpone());
  break_control->set_ignorable_break(get_break_ignorable());

  enabled = get_break_enabled();
}


//!
int
Break::get_timer_limit() const
{
  int rc;
  configurator->get_value_default(timer_prefix + CFG_KEY_TIMER_LIMIT, &rc,
                                  default_config[break_id].limit);
  return rc;
}


//!
void
Break::set_timer_limit(int n)
{
  configurator->set_value(timer_prefix + CFG_KEY_TIMER_LIMIT, n);
}


//!
int
Break::get_timer_auto_reset() const
{
  int rc;
  configurator->get_value_default(timer_prefix + CFG_KEY_TIMER_AUTO_RESET, &rc,
                                  default_config[break_id].auto_reset);
  return rc;
}


//!
void
Break::set_timer_auto_reset(int n)
{
  configurator->set_value(timer_prefix + CFG_KEY_TIMER_AUTO_RESET, n);
}


//!
string
Break::get_timer_reset_pred() const
{
  string rc;
  configurator->get_value_default(timer_prefix + CFG_KEY_TIMER_RESET_PRED, &rc,
                                  default_config[break_id].resetpred);
  return rc;
}


//!
void
Break::set_timer_reset_pred(string n)
{
  configurator->set_value(timer_prefix + CFG_KEY_TIMER_RESET_PRED, n);
}


//!
int
Break::get_timer_snooze() const
{
  int rc;
  configurator->get_value_default(timer_prefix + CFG_KEY_TIMER_SNOOZE, &rc,
                                  default_config[break_id].snooze);
  return rc;
}


//!
void
Break::set_timer_snooze(int n)
{
  configurator->set_value(timer_prefix + CFG_KEY_TIMER_SNOOZE, n);
}


//!
string
Break::get_timer_monitor() const
{
  string rc;
  configurator->get_value(timer_prefix + CFG_KEY_TIMER_MONITOR, &rc);
  
  return rc;
}


//!
void
Break::set_timer_monitor(string n)
{
  configurator->set_value(timer_prefix + CFG_KEY_TIMER_MONITOR, n);
}


//!
bool
Break::get_timer_activity_sensitive() const
{
  bool rc = true;
  configurator->get_value_default(timer_prefix
                                  +CFG_KEY_TIMER_ACTIVITY_SENSITIVE,
                                  &rc, true);
  return rc;
}


void
Break::set_timer_activity_sensitive(bool b)
{
  configurator->set_value(timer_prefix + CFG_KEY_TIMER_ACTIVITY_SENSITIVE, b);
}


int
Break::get_break_max_preludes() const
{
  int rc;
  configurator->get_value_default(break_prefix + CFG_KEY_BREAK_MAX_PRELUDES,
                                  &rc,
                                  default_config[break_id].max_preludes);
  return rc;
}


//!
void
Break::set_break_max_preludes(int n)
{
  configurator->set_value(break_prefix + CFG_KEY_BREAK_MAX_PRELUDES, n);
}

//!
int
Break::get_break_max_postpone() const
{
  int rc;
  configurator->get_value_default(break_prefix + CFG_KEY_BREAK_MAX_POSTPONE,
                                  &rc,
                                  default_config[break_id].max_postpone);
  return rc;
}

//!
void
Break::set_break_max_postpone(int n)
{
  configurator->set_value(break_prefix + CFG_KEY_BREAK_MAX_POSTPONE, n);
}


//!
bool
Break::get_break_ignorable() const
{
  bool rc;
  configurator->get_value_default(break_prefix + CFG_KEY_BREAK_IGNORABLE,
                                  &rc,
                                  default_config[break_id].ignorable_break);
  return rc;
}


//!
void
Break::set_break_ignorable(bool b)
{
  configurator->set_value(break_prefix + CFG_KEY_BREAK_IGNORABLE, b);
}


//!
int
Break::get_break_exercises() const
{
  int rc;
  configurator->get_value_default(break_prefix + CFG_KEY_BREAK_EXERCISES, &rc,
                                  default_config[break_id].exercises);
  return rc;
}

void
Break::set_break_exercises(int n)
{
  configurator->set_value(break_prefix + CFG_KEY_BREAK_EXERCISES, n);
}




bool
Break::get_break_enabled() const
{
  bool rc;
  configurator->get_value_default(break_prefix + CFG_KEY_BREAK_ENABLED, &rc,
                                  true);
  return rc;
}


void
Break::set_break_enabled(bool b)
{
  configurator->set_value(break_prefix + CFG_KEY_BREAK_ENABLED, b);
}


bool
Break::starts_with(string &key, string prefix, string &name)
{
  TRACE_ENTER_MSG("Break::starts_with", key << " " << prefix);
  bool ret = false;
  
  // Search prefix (just in case some Configurator added a leading /)
  string::size_type pos = key.rfind(prefix);
  
  if (pos != string::npos)
    {
      TRACE_MSG(pos);
      key = key.substr(pos + prefix.length());
      pos = key.find('/');

      if (pos != string::npos)
        {
          name = key.substr(0, pos);
          key = key.substr(pos + 1);
        }
      ret = true;
    }

  TRACE_EXIT();
  return ret;
}



//! Notification that the configuration changed.
void
Break::config_changed_notify(string key)
{
  TRACE_ENTER_MSG("Break::config_changed_notify", key);
  string name;

  if (starts_with(key, CFG_KEY_BREAK_PREFIX, name))
    {
      TRACE_MSG("break: " << name);
      load_break_control_config();
    }
  else if (starts_with(key, CFG_KEY_TIMER_PREFIX, name))
    {
      TRACE_MSG("timer: " << name);
      load_timer_config();
      load_timer_monitor_config();
    }
  TRACE_EXIT();
}

// BreakControl.cc
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

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"

#include <string>

#include "BreakControl.hh"
#include "Statistics.hh"

#include "GUIFactoryInterface.hh"
#include "PreludeWindowInterface.hh"
#include "BreakWindowInterface.hh"

#include "ActivityMonitorInterface.hh"
#include "ControlInterface.hh"
#include "TimerInterface.hh"


//! Construct a new Break Controller.
/*!
 *  \param id ID of the break this BreakControl controls.
 *  \param c pointer to control interface.
 *  \param factory pointer to the GUI window factory used to create the break
 *          windows.
 *  \param timer pointer to the interface of the timer that belongs to this break.
 */
BreakControl::BreakControl(GUIControl::BreakId id, ControlInterface *c,
                           GUIFactoryInterface *factory, TimerInterface *timer) :
  break_id(id),
  core_control(c),
  gui_factory(factory),
  break_timer(timer),
  break_window(NULL),
  prelude_window(NULL),
  break_stage(STAGE_NONE),
  prelude_time(0),
  final_prelude(false),
  forced_break(false),
  prelude_count(0),
  number_of_preludes(2),
  force_after_prelude(true),
  insist_break(true),
  ignorable_break(true),
  break_window_destroy(false),
  prelude_window_destroy(false),
  delay_window_destroy(false),
  insist_policy(INSIST_POLICY_HALT),
  active_insist_policy(INSIST_POLICY_INVALID)
{
  set_insist_break(insist_break);
  set_ignorable_break(ignorable_break);

  assert(break_timer != NULL);
}


//! Destructor.
BreakControl::~BreakControl()
{
  collect_garbage();
}


//! Periodic heartbeat.
void
BreakControl::heartbeat()
{
  TRACE_ENTER("BreakControl::heartbeat");

  collect_garbage();
  
  prelude_time++;

  ActivityMonitorInterface *monitor = core_control->get_activity_monitor();
  ActivityState activity_state = monitor->get_current_state();

  TRACE_MSG("state = " << activity_state << " " << ACTIVITY_ACTIVE << " " << prelude_time);
  TRACE_MSG("stage = " << break_stage);

  time_t idle = break_timer->get_elapsed_idle_time();
  TRACE_MSG("idle = " << idle);

  TimerInterface::TimerState tstate = break_timer->get_state();
  TRACE_MSG("tstate = " << tstate);
  
  bool is_idle = tstate == TimerInterface::STATE_STOPPED;

  switch (break_stage)
    {
    case STAGE_NONE:
      break;
      
    case STAGE_SNOOZED:
      break;
      
    case STAGE_PRELUDE:
      {
        TRACE_MSG("prelude count = " << prelude_count);

        assert(prelude_window != NULL);
        update_prelude_window();
        prelude_window->refresh();

        if (is_idle)
          {
            if (prelude_time >= 5)
              {
                // Show at least 5 s.
                // FIXME: MUST be at least the noise threshold.. check this
                
                goto_stage(STAGE_TAKING);
              }
          }
        else if (prelude_time == 30)
          {
            if (force_after_prelude && final_prelude)
              {
                goto_stage(STAGE_TAKING);
              }
            else
              {
                goto_stage(STAGE_SNOOZED);
              }
          }
        else if (prelude_time == 20)
          {
            assert(prelude_window != NULL);
            prelude_window->set_frame(2);
            prelude_window->refresh();
          }
        else if (prelude_time == 10)
          {
            assert(prelude_window != NULL);
            prelude_window->set_frame(1);
            prelude_window->refresh();
          }
      } 
      break;
        
    case STAGE_TAKING:
      {
        // We go back to prelude IF
        // 1) the user is NOT idle, and
        // 2) this is NO forced (user initiated) break, and
        // 3) we don't have number_of_preludes set (i.e. >= 0)
        // 4) we hasn't reached the number_of_preludes
        if (!is_idle && !forced_break && !final_prelude && !insist_break)
          {
            goto_stage(STAGE_PRELUDE);
          }
        else
          {
            update_break_window();
            
            assert(break_window != NULL);
            break_window->refresh();
          }
      }
      break;
    }

  TRACE_EXIT();
}
  

//! Initiates the specified break stage.
void
BreakControl::goto_stage(BreakStage stage)
{
  TRACE_ENTER_MSG("BreakControl::goto_stage", stage);
  switch (stage)
    {
    case STAGE_NONE:
      break_window_stop();
      prelude_window_stop();
      {
        defrost();
        
        if (break_stage == STAGE_TAKING)
          {
            time_t idle = break_timer->get_elapsed_idle_time();
            time_t reset = break_timer->get_auto_reset();

            if (idle >= reset)
              {
                Statistics *stats = Statistics::get_instance();
                stats->increment_counter(break_id, Statistics::STAT_TYPE_TAKEN);
              }
          }
      }
      break;
      
    case STAGE_SNOOZED:
      {
        break_window_stop();
        prelude_window_stop();
        // break_timer->snooze_timer();

        defrost();
      }
      break;

    case STAGE_PRELUDE:
      {
        prelude_count++;
        prelude_time = 0;
        break_window_stop();

        prelude_window_start();
        prelude_window->refresh();
      }
      break;
        
    case STAGE_TAKING:
      {
        prelude_window_stop();

        break_window_start();
        assert(break_window != NULL);
        break_window->refresh();

        if (insist_break)
          {
            freeze();
          }
      }
      break;
    }

  break_stage = stage;
  TRACE_EXIT();
}


//! Updates the contents of the prelude window.
void
BreakControl::update_prelude_window()
{
  assert(prelude_window != NULL);
  prelude_window->set_progress(prelude_time, 29);
}


//! Updates the contents of the break window.
void
BreakControl::update_break_window()
{
  assert(break_timer != NULL);
  time_t idle = break_timer->get_elapsed_idle_time();
  time_t duration = break_timer->get_auto_reset();

  if (idle > duration)
    {
      idle = duration;
    }
  assert(break_window != NULL);
  break_window->set_progress(idle, duration);
}


//! Starts the break.
void
BreakControl::start_break()
{
  TRACE_ENTER("BreakControl::start_break");
  
  forced_break = false;
  prelude_time = 0;

  // Idle until proven guilty.
  ActivityMonitorInterface *monitor = core_control->get_activity_monitor();
  TRACE_MSG("prelude count = " << prelude_count << " " << number_of_preludes);

  final_prelude = number_of_preludes >= 0 && prelude_count + 1 >= number_of_preludes;

  TRACE_MSG("final_prelude = " << final_prelude);
  
  if (!force_after_prelude && number_of_preludes >= 0 && prelude_count >= number_of_preludes)
    {
      goto_stage(STAGE_SNOOZED);
    }
  else
    {
      
      monitor->force_idle();
      break_timer->stop_timer();
  
      Statistics *stats = Statistics::get_instance();
      stats->increment_counter(break_id, Statistics::STAT_TYPE_PROMPTED);
      
      if (prelude_count == 0)
        {
          stats->increment_counter(break_id, Statistics::STAT_TYPE_UNIQUE_BREAKS);
        }
      
      goto_stage(STAGE_PRELUDE);
    }

  TRACE_EXIT();
}


//! Starts the break without preludes.
void
BreakControl::force_start_break()
{
  TRACE_ENTER("BreakControl::force_start_break");

  forced_break = true;
  prelude_time = 0;

  goto_stage(STAGE_TAKING);

  TRACE_EXIT();
}


//! Stops the break.
/*!
 *  Stopping a break will reset the "number of presented preludes" counter. So,
 *  wrt, "max-preludes", the break will start over when ik comes back.
 */
void
BreakControl::stop_break()
{
  TRACE_ENTER("BreakControl::stop_break");

  suspend_break();
  prelude_count = 0;

  TRACE_EXIT();
}


//! Suspend the break.
/*!
 *  A suspended break will come back after snooze time. The number of times the
 *  break will come back can be defined with set_max_preludes.
 */
void
BreakControl::suspend_break()
{
  TRACE_ENTER("BreakControl::suspend_break");

  goto_stage(STAGE_NONE);
  defrost();
  
  TRACE_EXIT();
}


//! Sets the prompt text shown in the prelude window.
void
BreakControl::set_prelude_text(string text)
{
  prelude_text = text;
}


//! Does the controller need a heartbeat?
bool
BreakControl::need_heartbeat()
{
  return
    break_stage != STAGE_NONE &&
    break_stage != STAGE_SNOOZED; 
}


//! Is the break active ?
BreakInterface::BreakState
BreakControl::get_break_state()
{
  TRACE_ENTER("BreakControl::get_break_state")
  BreakState ret = BREAK_INACTIVE;

  if (break_stage == STAGE_NONE)
    {
      ret = BREAK_INACTIVE;
    }
  else if (break_stage == STAGE_SNOOZED)
    {
      ret = BREAK_INACTIVE; // FIXME: actually: SUSPENDED;
    }
  else
    {
      ret = BREAK_ACTIVE;
    }
  TRACE_RETURN(ret);
  return ret;
}


//! Postpones the active break.
/*!
 *  Postponing a break does not reset the break timer. The break prelude window
 *  will re-appear after snooze time.
 */
void
BreakControl::postpone_break()
{
  // Snooze the timer.
  break_timer->snooze_timer();
  
  Statistics *stats = Statistics::get_instance();
  stats->increment_counter(break_id, Statistics::STAT_TYPE_POSTPONED);

  // and stop the break.
  stop_break();
}


//! Skips the active break.
/*!
 *  Skipping a break resets the break timer.
 */
void
BreakControl::skip_break()
{
  // Reset the restbreak timer.
  break_timer->reset_timer();

  Statistics *stats = Statistics::get_instance();
  stats->increment_counter(break_id, Statistics::STAT_TYPE_SKIPPED);
  
  // and stop the break.
  stop_break();
}


//! Sets the 'force-after-preludes' flags
/*!
 *  After the maximum number of preludes, the break either stops bothering the
 *  user (force-after-preludes = false), or forces a break (force-after-preludes = true)
 */
void
BreakControl::set_force_after_preludes(bool f)
{
  force_after_prelude = f;
}


//! Sets the maximum number of preludes. 
/*!
 *  After the maximum number of preludes, the break either stops bothering the
 *  user, or forces a break. This can be set with set_force_after_preludes.
 */
void
BreakControl::set_max_preludes(int m)
{
  number_of_preludes = m;
}


//! Sets the insist-break flags
/*!
 *  A break that has 'insist' set, locks the keyboard during the break..
 */
void
BreakControl::set_insist_break(bool i)
{
  TRACE_ENTER_MSG("BreakControl::set_insist_break", i)
  insist_break = i;
  if (break_window != NULL)
    {
      break_window->set_insist_break(insist_break);
    }
  TRACE_EXIT();
}


//! Sets the ignorable-break flags
/*!
 *  A break that is ignorable had a skip/postpone button.
 */
void
BreakControl::set_ignorable_break(bool i)
{
  ignorable_break = i;
}


void
BreakControl::set_insist_policy(InsistPolicy p)
{
  insist_policy = p;
}


//! Creates and shows the break window.
void
BreakControl::break_window_start()
{
  if (break_window == NULL)
    {
      assert(gui_factory != NULL);
      break_window = gui_factory->create_break_window(break_id, forced_break ? true : ignorable_break);
    }

  break_window_destroy = false;
  
  assert(break_window != NULL);
  break_window->set_break_response(this);
  break_window->set_insist_break(insist_break);
  
  update_break_window();
  
  break_window->start();
}


//! Removes the break windows.
void
BreakControl::break_window_stop()
{
  if (break_window != NULL)
    {
      break_window->stop();
      break_window_destroy = true;
      delay_window_destroy = true;
    }
}


//! Creates and shows the prelude window.
void
BreakControl::prelude_window_start()
{
  if (prelude_window == NULL)
    {
      assert(gui_factory != NULL);
      prelude_window = gui_factory->create_prelude_window();
      assert(prelude_window != NULL);
    }
  
  prelude_window_destroy = false;

  prelude_window->set_frame(0);
  prelude_window->set_text(prelude_text);

  if (!final_prelude)
    {
      prelude_window->set_progress_text("Disappears in ");
    }
  else if (force_after_prelude) // && final_prelude
    {
      prelude_window->set_progress_text("Break in ");
    }
  else // final_prelude && ! force_after_prelude
    {
      prelude_window->set_progress_text("SILENT in ");
    }
  
  update_prelude_window();
  
  prelude_window->start();
}


//! Removes the prelude window.
void
BreakControl::prelude_window_stop()
{
  if (prelude_window != NULL)
    {
      prelude_window->stop();
      prelude_window_destroy = true;
      delay_window_destroy = true;
    }
}


//! Destroy the break/prelude windows, if requested.
void
BreakControl::collect_garbage()
{
  if (delay_window_destroy)
    {
      delay_window_destroy = false;
    }
  else
    {
      if (prelude_window_destroy)
        {
          prelude_window->destroy();
          prelude_window = NULL;
          prelude_window_destroy = false;
        }
  
      if (break_window_destroy)
        {
          break_window->destroy();
          break_window = NULL;
          break_window_destroy = false;
        }
    }
}


void
BreakControl::freeze()
{
  TRACE_ENTER("BreakControl::freeze");
  switch (insist_policy)
    {
    case INSIST_POLICY_SUSPEND:
      {
        TRACE_MSG("suspending monitor");
        ActivityMonitorInterface *monitor = core_control->get_activity_monitor();
        monitor->suspend();
      }
      break;
    case INSIST_POLICY_HALT:
      {
        TRACE_MSG("freezing timer");
        GUIControl *gui_control = GUIControl::get_instance();
        gui_control->set_freeze_all_breaks(true);
      }
      break;
    case INSIST_POLICY_RESET:
      
    default:
      break;
    }

  active_insist_policy = insist_policy;
  TRACE_EXIT();
}

void
BreakControl::defrost()
{
  TRACE_ENTER("BreakControl::defrost");
  switch (active_insist_policy)
    {
    case INSIST_POLICY_SUSPEND:
      {
        ActivityMonitorInterface *monitor = core_control->get_activity_monitor();
        monitor->resume();
      }
      break;
    case INSIST_POLICY_HALT:
      {
        TRACE_MSG("defrosting timer");
        GUIControl *gui_control = GUIControl::get_instance();
        gui_control->set_freeze_all_breaks(false);
      }
      break;
      
    default:
      break;
    }

  active_insist_policy = INSIST_POLICY_INVALID;
  
  TRACE_EXIT();
}

// TimerPreferencesPanel.cc --- Preferences widgets for a timer
//
// Copyright (C) 2002 Raymond Penners <raymond@dotsphinx.com>
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

// TODO: only when needed.
#define NOMINMAX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"

#include <unistd.h>
#include "Timer.hh"
#include "DayTimePred.hh"
#include "TimeEntry.hh"
#include "TimerPreferencesPanel.hh"
#include "ControlInterface.hh"
#include "Configurator.hh"

using std::cout;
using SigC::slot;


TimerPreferencesPanel::TimerPreferencesPanel(GUIControl::TimerId t)
  : Gtk::HBox(false, 6),
    max_prelude_adjustment(0, 1, 100)
{
  TRACE_ENTER("TimerPreferencesPanel::TimerPreferencesPanel");
  timer_id = t;
  timer = &GUIControl::get_instance()->timers[t];

  // Frames
  Gtk::Frame *prelude_frame = manage(create_prelude_frame());
  Gtk::Frame *timers_frame = manage(create_timers_frame());
  Gtk::Frame *opts_frame = manage(create_options_frame());

  // VBox
  Gtk::VBox *vbox = new Gtk::VBox(false, 6);
  vbox->pack_start(*timers_frame, false, false, 0);
  vbox->pack_start(*opts_frame, false, false, 0);
  

  // Overall box
  pack_start(*vbox, false, false, 0);
  pack_start(*prelude_frame, false, false, 0);

  set_border_width(6);

  TRACE_EXIT();
}


TimerPreferencesPanel::~TimerPreferencesPanel()
{
  TRACE_ENTER("TimerPreferencesPanel::~TimerPreferencesPanel");
  // FIXME: disconnect signals?
  TRACE_EXIT();
}


Gtk::Frame *
TimerPreferencesPanel::create_prelude_frame()
{
  // Prelude frame
  Gtk::Frame *prelude_frame = new Gtk::Frame("Break prompting");
  prelude_cb = manage(new Gtk::CheckButton("Prompt before breaking"));
  TimerInterface *itimer = timer->timer;
  string pfx = GUIControl::CFG_KEY_BREAK + itimer->get_id();
  Configurator *cfg = GUIControl::get_instance()->get_configurator();
  int maxprel;
  bool b = cfg->get_value(pfx + GUIControl::CFG_KEY_BREAK_MAX_PRELUDES,
                          &maxprel);
  prelude_cb->signal_toggled()
    .connect(SigC::slot(*this,
                        &TimerPreferencesPanel::on_preludes_active_toggled));

  has_max_prelude_cb = manage(new Gtk::CheckButton
                              ("Maximum number of prompts:"));
  has_max_prelude_cb->signal_toggled()
    .connect(SigC::slot(*this,
                        &TimerPreferencesPanel::on_preludes_maximum_toggled));

  max_prelude_spin = manage(new Gtk::SpinButton(max_prelude_adjustment));

  force_after_prelude_cb = manage(new Gtk::CheckButton
                                  ("Enforce break after maximum exceeded"));
  force_after_prelude_cb->signal_toggled()
    .connect(SigC::slot(*this,
                        &TimerPreferencesPanel::on_preludes_force_toggled));

  Gtk::VBox *prelude_vbox = new Gtk::VBox(false, 6);
  prelude_vbox->set_border_width(6);
  prelude_vbox->pack_start(*prelude_cb, false, false, 0);
  prelude_vbox->pack_start(*has_max_prelude_cb, false, false, 0);
  prelude_vbox->pack_start(*max_prelude_spin, false, false, 0);
  prelude_vbox->pack_start(*force_after_prelude_cb, false, false, 0);
  prelude_frame->add(*prelude_vbox);

  return prelude_frame;
}

Gtk::Frame *
TimerPreferencesPanel::create_options_frame()
{
  TimerInterface *itimer = timer->timer;
  
  // Insists option
  bool insists = timer->get_break_insisting();
  insists_cb = manage(new Gtk::CheckButton("Block user input"));
  insists_cb->signal_toggled()
    .connect(SigC::slot(*this, &TimerPreferencesPanel::on_insists_toggled));
  insists_cb->set_active(insists);

  // Monitor
  Configurator *cfg = GUIControl::get_instance()->get_configurator();
  string tpfx = ControlInterface::CFG_KEY_TIMER + itimer->get_id();
  if (timer_id == GUIControl::TIMER_ID_DAILY_LIMIT)
    {
      monitor_cb
        = manage(new Gtk::CheckButton("Regard micro-pauses as activity"));
      monitor_cb->signal_toggled()
        .connect(SigC::slot(*this, &TimerPreferencesPanel::on_monitor_toggled));
      string monitor_name;
      bool b = cfg->get_value
        (tpfx + ControlInterface::CFG_KEY_TIMER_MONITOR, &monitor_name);
      monitor_cb->set_active(b && monitor_name != "");
    }
  else
    monitor_cb = NULL;

  // Ignorable
  bool ignorable = timer->get_break_ignorable();
  ignorable_cb = manage(new Gtk::CheckButton
                        ("Show 'Postpone' and 'Skip' button"));
  ignorable_cb->signal_toggled()
    .connect(SigC::slot(*this, &TimerPreferencesPanel::on_ignorable_toggled));
  ignorable_cb->set_active(ignorable);
  
  // Options table
  Gtk::Table *opts_table = manage(new Gtk::Table(2,2, false));
  int y = 0;
  opts_table->attach(*insists_cb, 1, 2, y, y+1,
                     Gtk::FILL, Gtk::SHRINK);
  y++;
  opts_table->attach(*ignorable_cb, 1, 2, y, y+1,
                     Gtk::FILL, Gtk::SHRINK);
  y++;
  if (monitor_cb)
    {
      opts_table->attach(*monitor_cb, 1, 2, y, y+1,
                         Gtk::FILL, Gtk::SHRINK);
      y++;
    }

  // Options frame
  Gtk::Frame *opts_frame = new Gtk::Frame("Options");
  opts_table->set_border_width(6);
  opts_frame->add(*opts_table);

  return opts_frame;
}

Gtk::Frame *
TimerPreferencesPanel::create_timers_frame()
{
  // Snooze time
  TimerInterface *itimer = timer->timer;
  Gtk::Label *snooze_lab = manage(new Gtk::Label("Post-pone time"));
  snooze_tim = manage(new TimeEntry());
  snooze_tim->set_value (itimer->get_snooze());
  snooze_tim->signal_value_changed()
    .connect(SigC::slot(*this, &TimerPreferencesPanel::on_snooze_changed));

  // Auto-reset time
  const char *auto_reset_txt;
  time_t auto_reset_value;
  if (timer_id == GUIControl::TIMER_ID_DAILY_LIMIT)
    {
      auto_reset_txt = "Resets at";
      DayTimePred *pred = (DayTimePred *) itimer->get_auto_reset_predicate();
      assert(pred);
      auto_reset_value = pred->get_time_offset();
    }
  else
    {
      auto_reset_txt = "Pause duration";
      auto_reset_value = itimer->get_auto_reset();
    }
  
  Gtk::Label *auto_reset_lab = manage(new Gtk::Label(auto_reset_txt));
  auto_reset_tim = manage(new TimeEntry());
  auto_reset_tim->set_value (auto_reset_value);
  auto_reset_tim->signal_value_changed()
    .connect(SigC::slot(*this, &TimerPreferencesPanel::on_auto_reset_changed));

  // Limit time
  Gtk::Label *limit_lab = manage(new Gtk::Label("Time before break"));
  limit_tim = manage(new TimeEntry());
  limit_tim->set_value (itimer->get_limit());
  limit_tim->signal_value_changed()
    .connect(SigC::slot(*this, &TimerPreferencesPanel::on_limit_changed));

  // Timers table
  Gtk::Table *timers_table = manage(new Gtk::Table(3, 2, false));
  timers_table->set_row_spacings(2);
  timers_table->set_col_spacings(6);
  int y = 0;
  timers_table->attach(*limit_lab, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  timers_table->attach(*limit_tim, 1, 2, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  timers_table->attach(*auto_reset_lab, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  timers_table->attach(*auto_reset_tim, 1, 2, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  timers_table->attach(*snooze_lab, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  timers_table->attach(*snooze_tim, 1, 2, y, y+1, Gtk::SHRINK, Gtk::SHRINK);

  // Timers frame
  Gtk::Frame *timers_frame = new Gtk::Frame("Timers");
  timers_table->set_border_width(6);
  timers_frame->add(*timers_table);
  return timers_frame;
}


void
TimerPreferencesPanel::config_changed_notify(string key)
{
}

void
TimerPreferencesPanel::on_preludes_active_toggled()
{
  bool is_active = prelude_cb->get_active();
}

void
TimerPreferencesPanel::on_preludes_maximum_toggled()
{
}

void
TimerPreferencesPanel::on_preludes_force_toggled()
{
}



void
TimerPreferencesPanel::on_auto_reset_changed()
{
  TRACE_ENTER("TimerPreferencesPanel::on_auto_reset_changed");
  string key;
  key = ControlInterface::CFG_KEY_TIMER + timer->timer->get_id();
  time_t val = auto_reset_tim->get_value();
  if (timer_id == GUIControl::TIMER_ID_DAILY_LIMIT)
    {
      key +=  ControlInterface::CFG_KEY_TIMER_RESET_PRED;
      DayTimePred pred;
      pred.init(val/(60*60), (val/60)%60);
      GUIControl::get_instance()->get_configurator()->set_value(key, pred.to_string());
    }
  else
    {
      key +=  ControlInterface::CFG_KEY_TIMER_AUTO_RESET;
      GUIControl::get_instance()->get_configurator()
        ->set_value(key, val);
    }
  TRACE_EXIT();
}

void
TimerPreferencesPanel::on_snooze_changed()
{
  TRACE_ENTER("TimerPreferencesPanel::on_snooze_changed");
  string key;
  key = ControlInterface::CFG_KEY_TIMER + timer->timer->get_id()
    + ControlInterface::CFG_KEY_TIMER_SNOOZE;
  GUIControl::get_instance()->get_configurator()
    ->set_value(key, snooze_tim->get_value());

  TRACE_EXIT();
}

void
TimerPreferencesPanel::on_limit_changed()
{
  TRACE_ENTER("TimerPreferencesPanel::on_limit_changed");
  string key;
  key = ControlInterface::CFG_KEY_TIMER + timer->timer->get_id()
    + ControlInterface::CFG_KEY_TIMER_LIMIT;
  GUIControl::get_instance()->get_configurator()
    ->set_value(key, limit_tim->get_value());

  TRACE_EXIT();
}

void
TimerPreferencesPanel::on_insists_toggled()
{
  timer->set_break_insisting(insists_cb->get_active());
}

void
TimerPreferencesPanel::on_monitor_toggled()
{
  string key = ControlInterface::CFG_KEY_TIMER + timer->timer->get_id()
    + ControlInterface::CFG_KEY_TIMER_MONITOR;
  string val;

  if (monitor_cb->get_active())
    {
      val = GUIControl::get_instance()
        ->timers[GUIControl::TIMER_ID_MICRO_PAUSE]
        .timer->get_id();
    }

  GUIControl::get_instance()->get_configurator()->set_value(key, val);
}

void
TimerPreferencesPanel::on_ignorable_toggled()
{
  timer->set_break_ignorable(ignorable_cb->get_active());
}

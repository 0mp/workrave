// TimerPreferencesPanel.hh --- Preferences widgets for a timer
//
// Copyright (C) 2002, 2003 Raymond Penners <raymond@dotsphinx.com>
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

#ifndef TIMERPREFERENCESPANEL_HH
#define TIMERPREFERENCESPANEL_HH

#include "preinclude.h"

#ifdef HAVE_CONFIG
#include "config.h"
#endif

#include <stdio.h>
#include <string>

#include <gtkmm/box.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/adjustment.h>

#include "CoreInterface.hh"

class TimeEntry;

namespace Gtk
{
  class CheckButton;
  class SpinButton;
}

class TimerPreferencesPanel
  : public Gtk::VBox
{
public:  
  TimerPreferencesPanel(BreakId timer, Glib::RefPtr<Gtk::SizeGroup> size_group);
  ~TimerPreferencesPanel();
  
private:
  void on_snooze_changed();
  void on_auto_reset_changed();
  void on_limit_changed();
  void on_ignorable_toggled();
  void on_insists_toggled();
  void on_monitor_toggled();
  void on_preludes_active_toggled();
  void on_preludes_maximum_toggled();
  void on_preludes_maximum_changed();
#ifdef HAVE_EXERCISES
  void on_exercises_changed();
#endif
  Gtk::Widget *create_prelude_panel();
  Gtk::Widget *create_options_panel();
  Gtk::Widget *create_timers_panel(Glib::RefPtr<Gtk::SizeGroup> size_group);
  void set_prelude_sensitivity();

  void on_enabled_toggled();
  void enable_buttons();
  
  BreakId break_id;
  BreakInterface *break_data;

  Gtk::CheckButton *insists_cb;
  Gtk::CheckButton *ignorable_cb;
  Gtk::CheckButton *monitor_cb;
  Gtk::CheckButton *prelude_cb;
  Gtk::CheckButton *has_max_prelude_cb;
  TimeEntry *limit_tim, *auto_reset_tim, *snooze_tim;
  Gtk::SpinButton *max_prelude_spin;
  Gtk::Adjustment max_prelude_adjustment;
  Gtk::CheckButton *enabled_cb;
#ifdef HAVE_EXERCISES
  Gtk::SpinButton *exercises_spin;
  Gtk::Adjustment exercises_adjustment;
#endif
};

#endif // TIMERPREFERENCESPANEL_HH

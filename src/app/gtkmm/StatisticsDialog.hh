// PreferencesDialog.hh --- Statistics Dialog
//
// Copyright (C) 2002, 2003 Rob Caelers & Raymond Penners
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

#ifndef STATISTICSDIALOG_HH
#define STATISTICSDIALOG_HH

#include "preinclude.h"
#include <sstream>

#include "StatisticsInterface.hh"

namespace Gtk
{
  class Label;
  class Button;
  class Calendar;
  class Notebook;
}

#include "Hig.hh"

class StatisticsDialog : public HigDialog
{
public:  
  StatisticsDialog();
  ~StatisticsDialog();

  static const int BREAK_STATS = 7;
  
  int run();
  
private:
  /** Stats */
  StatisticsInterface *statistics;
  
  /** Labels for break stats. */
  Gtk::Label *break_labels[BREAK_ID_SIZEOF][9];

  /** Labels for break stats. */
  Gtk::Label *activity_labels[5];
  
  /** Labels for break stats. */
  Gtk::Label *daily_usage_label;

  /** Labels indicating the start time of the visible data. */
  Gtk::Label *date_label;

  /** Calendar */
  Gtk::Calendar *calendar;

  /** Forward button */
  Gtk::Button *forward_btn;

  /** Back button */
  Gtk::Button *back_btn;

  /** Last button */
  Gtk::Button *last_btn;

  /** First button */
  Gtk::Button *first_btn;

  void init_gui();
  void select_day(int day);
  
  void create_break_page(Gtk::Notebook *tnotebook);
  void create_activity_page(Gtk::Notebook *tnotebook);

  void stream_distance(std::stringstream &stream, int pixels);
  void get_calendar_day_index(int &idx, int &next, int &prev);
  void set_calendar_day_index(int idx);
  void on_calendar_month_changed();
  void on_calendar_day_selected();
  void on_history_go_back();
  void on_history_go_forward();
  void on_history_goto_last();
  void on_history_goto_first();
  void display_calendar_date();
  void display_statistics(StatisticsInterface::DailyStats *stats);
  void clear_display_statistics();
  
  bool on_timer();
};

#endif // STATISTICSWINDOW_HH

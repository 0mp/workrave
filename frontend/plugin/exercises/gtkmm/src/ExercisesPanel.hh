// ExercisesPanel.hh --- Exercises panel
//
// Copyright (C) 2002, 2003, 2004, 2005, 2006 Raymond Penners <raymond@dotsphinx.com>
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

#ifndef EXERCISES_PANEL_HH
#define EXERCISES_PANEL_HH

#ifdef HAVE_CONFIG
#include "config.h"
#endif

#ifdef HAVE_EXERCISES

#include "preinclude.h"
#include "Exercise.hh"

#include <gtkmm/box.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/frame.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>

#ifdef HAVE_CHIROPRAKTIK
#include "W32Mp3Player.hh"
#endif

class ExercisesPanel :
  public
#ifdef HAVE_CHIROPRAKTIK
Gtk::VBox
#else
Gtk::HBox
#endif
{
public:  
  ExercisesPanel(Gtk::HButtonBox *dialog_action_area);
  ~ExercisesPanel();

  void set_exercise_count(int num);
  SigC::Signal0<void> &signal_stop() { return stop_signal; }

protected:
  void on_realize();

private:
  void reset();
  void on_go_back();
  void on_go_forward();
  void on_pause();
  void on_stop();
#ifdef HAVE_CHIROPRAKTIK
  void on_speak_volume();
  void refresh_speak_volume();
  bool on_ad_clicked(GdkEventButton *event);
  bool on_ad_leave_enter(GdkEventCrossing *event);
#endif
  
  void heartbeat();
  void start_exercise();
  void show_image();
  void refresh_progress();
  void refresh_sequence();
  void refresh_pause();
  int adjust_exercises_pointer(int inc)
  {
    int ret = exercises_pointer;
#ifdef HAVE_CHIROPRAKTIK
    if (ret < 0) { ret = exercises_pointer = time(NULL) % exercises.size();}
#endif
    exercises_pointer += inc;
    if (exercises.size() != 0)
      {
        exercises_pointer %= exercises.size();
      }
    return ret;
  }
  
  Gtk::Frame image_frame;
  Gtk::Image image;
  Gtk::ProgressBar progress_bar;
  Gtk::TextView description_text;
  Gtk::ScrolledWindow description_scroll;
  Gtk::Button *back_button;
  Gtk::Button *pause_button;
  Gtk::Button *forward_button;
#ifdef HAVE_CHIROPRAKTIK
  Gtk::Button *speak_volume_button;
  W32Mp3Player mp3_player;
#endif
  Glib::RefPtr<Gtk::SizeGroup> size_group;
  const std::list<Exercise> exercises;
  std::list<Exercise>::const_iterator exercise_iterator;
  std::list<Exercise::Image>::const_iterator image_iterator;
  SigC::Connection heartbeat_signal;
  int exercise_time;
  int seq_time;
  bool paused;
  bool stopped;
  SigC::Signal0<void> stop_signal;
  bool standalone;
  int exercise_num;
  int exercise_count;
  static int exercises_pointer;
};

#endif // HAVE_EXERCISES

#endif // EXERCISES_PANEL_HH

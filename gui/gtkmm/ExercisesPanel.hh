// ExercisesPanel.hh --- Exercises panel
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

#ifndef EXERCISES_PANEL_HH
#define EXERCISES_PANEL_HH

#include "config.h"

#ifdef HAVE_EXERCISES

#include "preinclude.h"
#include "Exercise.hh"
#include <gtkmm.h>

class ExercisesPanel : public Gtk::HBox
{
public:  
  ExercisesPanel();
  ~ExercisesPanel();

private:
  void on_go_back();
  void on_go_forward();
  void on_pause();

  Gtk::Frame image_frame;
  Gtk::Image image;
  Gtk::ProgressBar progress_bar;
  Gtk::TextView text_view;
  Gtk::Button *back_button;
  Gtk::Button *pause_button;
  Gtk::Button *forward_button;
  Gtk::HBox button_box;
  Gtk::VBox image_box;
  std::list<Exercise> exercises;
};

#endif // HAVE_EXERCISES

#endif // EXERCISES_PANEL_HH

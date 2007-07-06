// RestBreakWindow.cc --- window for the microbreak
//
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 Rob Caelers & Raymond Penners
// All rights reserved.
//
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

const int TIMEOUT = 1000;

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "preinclude.h"

#include <unistd.h>

#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include "debug.hh"
#include "nls.h"

#include "Hig.hh"
#include "RestBreakWindow.hh"
#include "Text.hh"
#include "TimeBar.hh"
#include "Util.hh"
#include "WindowHints.hh"

#include "ITimer.hh"
#include "IBreak.hh"
#include "IBreakResponse.hh"
#include "GtkUtil.hh"
#include "Frame.hh"

#include "ICore.hh"
#include "CoreFactory.hh"
#include "IActivityMonitor.hh"

#ifdef HAVE_EXERCISES
#include "Exercise.hh"
#include "ExercisesPanel.hh"
#endif

const int MARGINX = 8;
const int MARGINY = 8;


//! Constructor
/*!
 *  \param control The controller.
 */
RestBreakWindow::RestBreakWindow(HeadInfo &head, bool ignorable,
                                 GUI::BlockMode mode) :
  BreakWindow(BREAK_ID_REST_BREAK, head, ignorable, mode),
  timebar(NULL),
  progress_value(0),
  progress_max_value(0),
  is_flashing(false)
{
  TRACE_ENTER("RestBreakWindow::RestBreakWindow");
  set_title(_("Rest break"));
  TRACE_EXIT();
}

Gtk::Widget *
RestBreakWindow::create_gui()
{
  // Add other widgets.
  Gtk::VBox *vbox = new Gtk::VBox(false, 6);

#ifdef HAVE_EXERCISES
  pluggable_panel = manage(new Gtk::HBox);
#endif
  
  vbox->pack_start(
#ifdef HAVE_EXERCISES
                   *pluggable_panel
#else
                   *create_info_panel()
#endif
                   , false, false, 0);

  // Timebar
  timebar = manage(new TimeBar);
  vbox->pack_start(*timebar, false, false, 6);

  Gtk::HButtonBox *button_box = create_break_buttons(true, false);
  if (button_box)
    {
      vbox->pack_end(*manage(button_box), Gtk::SHRINK, 6);
    }

  return vbox;
}


//! Destructor.
RestBreakWindow::~RestBreakWindow()
{
  TRACE_ENTER("RestBreakWindow::~RestBreakWindow");
  TRACE_EXIT();
}


//! Starts the restbreak.
void
RestBreakWindow::start()
{
  TRACE_ENTER("RestBreakWindow::start");
  init_gui();
#ifdef HAVE_EXERCISES
  if (get_exercise_count() > 0)
    {
      install_exercises_panel();
    }
  else
    {
      install_info_panel();
    }
#else
  set_ignore_activity(false);
#endif
  refresh();

  BreakWindow::start();

  TRACE_EXIT();
}


//! Period timer callback.
void
RestBreakWindow::refresh()
{ 
  BreakWindow::refresh();

  draw_time_bar();
}


void
RestBreakWindow::set_progress(int value, int max_value)
{
  progress_max_value = max_value;
  progress_value = value;
}


//! Draws the timer bar.
void
RestBreakWindow::draw_time_bar()
{
  timebar->set_progress(progress_value, progress_max_value);

  time_t time = progress_max_value - progress_value;
  char s[128];
  sprintf(s, _("Rest break for %s"), Text::time_to_string(time, true).c_str());
  
  timebar->set_text(s);

  ICore *core = CoreFactory::get_core();
  IActivityMonitor *monitor = core->get_activity_monitor();
  ActivityState state = monitor->get_current_state();
  if (frame != NULL)
    {
      if (state == ACTIVITY_ACTIVE && !is_flashing)
        {
          frame->set_frame_color(Gdk::Color("orange"));
          frame->set_frame_visible(true);
          frame->set_frame_flashing(500);
          is_flashing = true;
        }
      else if (state == ACTIVITY_IDLE && is_flashing)
        {
          frame->set_frame_flashing(0);
          frame->set_frame_visible(false);
          is_flashing = false;
        }
    }
  
  timebar->update();
}


Gtk::Widget *
RestBreakWindow::create_info_panel()
{
  Gtk::HBox *info_box = manage(new Gtk::HBox(false, 12));
  
  string icon = Util::complete_directory("rest-break.png", Util::SEARCH_PATH_IMAGES);
  Gtk::Image *info_img = manage(new Gtk::Image(icon));
  info_img->set_alignment(0.0, 0.0);
  Gtk::Label *info_lab =
    manage(new Gtk::Label());
  Glib::ustring txt = HigUtil::create_alert_text
    (_("Rest break"),
     _("This is your rest break. Make sure you stand up and\n"
       "walk away from your computer on a regular basis. Just\n"
       "walk around for a few minutes, stretch, and relax."));
  info_lab->set_markup(txt);
  info_box->pack_start(*info_img, false, false, 0);
  info_box->pack_start(*info_lab, false, true, 0);
  return info_box;
}

#ifdef HAVE_EXERCISES
void
RestBreakWindow::clear_pluggable_panel()
{
  TRACE_ENTER("RestBreakWindow::clear_pluggable_panel");
  Glib::ListHandle<Gtk::Widget *> children = pluggable_panel->get_children();
  if (children.size() > 0)
    {
      TRACE_MSG("Clearing");
      pluggable_panel->remove(*(*(children.begin())));
    }
  TRACE_EXIT();
}

int
RestBreakWindow::get_exercise_count()
{
  int ret = 0;
  
  if (Exercise::has_exercises())
    {
      ICore *core = CoreFactory::get_core();
      assert(core != NULL);
      
      ret = core->get_break(BREAK_ID_REST_BREAK)->get_break_exercises();
    }
  return ret;
}

void
RestBreakWindow::install_exercises_panel()
{
  if (head.count != 0)
    {
      install_info_panel();
    }
  else
    {
      set_ignore_activity(true);
      clear_pluggable_panel();
      ExercisesPanel *exercises_panel = manage(new ExercisesPanel(NULL));
      pluggable_panel->pack_start(*exercises_panel, false, false, 0);
      exercises_panel->set_exercise_count(get_exercise_count());
      exercises_panel->signal_stop().connect
        (MEMBER_SLOT(*this, &RestBreakWindow::install_info_panel));
      pluggable_panel->show_all();
      pluggable_panel->queue_resize();
      center();
    }
}

void
RestBreakWindow::install_info_panel()
{
  set_ignore_activity(false);
  clear_pluggable_panel();
  pluggable_panel->pack_start(*(create_info_panel()), false, false, 0);
  pluggable_panel->show_all();
  pluggable_panel->queue_resize();
  center();
}

#endif

void
RestBreakWindow::set_ignore_activity(bool i)
{
  ICore *core = CoreFactory::get_core();
  assert(core != NULL);

#ifdef WIN32
  if( !CoreFactory::get_configurator()->get_value( "advanced/force_focus", &force_focus ))
    i = true;
#endif
  
  core->set_insist_policy(i ?
                        ICore::INSIST_POLICY_IGNORE :
                        (block_mode != GUI::BLOCK_MODE_NONE
                         ? ICore::INSIST_POLICY_HALT
                         : ICore::INSIST_POLICY_RESET));
}

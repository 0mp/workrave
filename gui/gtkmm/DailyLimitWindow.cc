// DailyLimitWindow.cc --- window for the daily limit
//
// Copyright (C) 2001, 2002, 2003 Rob Caelers & Raymond Penners
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

#include "preinclude.h"

#include "nls.h"
#include "debug.hh"

#include "DailyLimitWindow.hh"
#include "WindowHints.hh"
#include "TimeBar.hh"
#include "TimerInterface.hh"
#include "BreakResponseInterface.hh"
#include "Util.hh"


//! Construct a new Micropause window.
DailyLimitWindow::DailyLimitWindow(bool ignorable) :
  insist_break(false)
{
  set_border_width(5);
  
  // Label
  Gtk::Label *label = manage
    (new Gtk::Label
     (_("You have reached your daily limit. Please stop working\n"
      "behind the computer. If your working day is not over yet,\n"
      "find something else to do, such as reviewing a document."
        )));

  // Icon
  string icon = Util::complete_directory("daily-limit.png", Util::SEARCH_PATH_IMAGES);
  Gtk::Image *img = manage(new Gtk::Image(icon));

  // HBox
  Gtk::HBox *hbox = manage(new Gtk::HBox(false, 6));
  hbox->pack_start(*img, false, false, 0);
  hbox->pack_start(*label, Gtk::EXPAND | Gtk::FILL, 10);

  // Overall vbox
  Gtk::VBox *box = manage(new Gtk::VBox(false, 12));
  box->pack_start(*hbox, Gtk::EXPAND | Gtk::FILL, 0);

  // Button box at the bottom.
  if (ignorable)
    {
      Gtk::HButtonBox *button_box = manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_END, 6));
      Gtk::Button *skipButton = manage(new Gtk::Button(_("Skip")));
      button_box->pack_end(*skipButton, Gtk::SHRINK, 0);
      Gtk::Button *postponeButton = manage(new Gtk::Button(_("Postpone")));
      button_box->pack_end(*postponeButton, Gtk::SHRINK, 0);
      GTK_WIDGET_UNSET_FLAGS(postponeButton->gobj(), GTK_CAN_FOCUS);
      GTK_WIDGET_UNSET_FLAGS(skipButton->gobj(), GTK_CAN_FOCUS);
      postponeButton->signal_clicked().connect(SigC::slot(*this, &DailyLimitWindow::on_postpone_button_clicked));
      skipButton->signal_clicked().connect(SigC::slot(*this, &DailyLimitWindow::on_skip_button_clicked));

      box->pack_start(*button_box, Gtk::EXPAND | Gtk::FILL, 0);
    }

  add(*box);

  // Need to realize window before it is shown
  // Otherwise, there is not gobj()...
  realize_if_needed();
  stick();
  
  // Set some window hints.
  WindowHints::set_skip_winlist(Gtk::Widget::gobj(), true);
  WindowHints::set_always_on_top(Gtk::Widget::gobj(), true);
  GTK_WIDGET_UNSET_FLAGS(Gtk::Widget::gobj(), GTK_CAN_FOCUS);
}


//! Destructor.
DailyLimitWindow::~DailyLimitWindow()
{
}


//! Starts the micropause.
void
DailyLimitWindow::start()
{
  center();
  show_all();

  if (insist_break)
    {
      grab();
    }

  present(); // After grab() please (Windows)
}


//! Stops the micropause.
void
DailyLimitWindow::stop()
{
  TRACE_ENTER("DailyLimitWindow::stop");

  ungrab();
  hide_all();

  TRACE_EXIT();
}


//! Self-Destruct
/*!
 *  This method MUST be used to destroy the objects through the
 *  BreakWindowInterface. it is NOT possible to do a delete on
 *  this interface...
 */
void
DailyLimitWindow::destroy()
{
  delete this;
}



void
DailyLimitWindow::set_progress(int value, int max_value)
{
  (void) value;
  (void) max_value;
  // FIXME: show progress until timer restart time?
}

void
DailyLimitWindow::set_insist_break(bool insist)
{
  insist_break = insist;
}


//! The postpone button was clicked.
void
DailyLimitWindow::on_postpone_button_clicked()
{
  if (break_response != NULL)
    {
      break_response->postpone_break();
    }
}


//! The skip button was clicked.
void
DailyLimitWindow::on_skip_button_clicked()
{
  if (break_response != NULL)
    {
      break_response->skip_break();
    }
}


//! Refreshes
void
DailyLimitWindow::refresh()
{
}

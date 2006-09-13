// KdeAppletWindow.hh --- X11 Applet Window
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
// $Id$
//

#ifndef KDEAPPLETWINDOW_HH
#define KDEAPPLETWINDOW_HH

#include "preinclude.h"
#include <stdio.h>

#include "AppletWindow.hh"

#ifdef HAVE_GTKMM24
#include <sigc++/compatibility.h>
#endif

#include <gtkmm/bin.h>
#include <gtkmm/menu.h>
#include <gtkmm/plug.h>
#include <gtkmm/eventbox.h>

class TimerBoxControl;
class TimerBoxGtkView;
class AppletControl;

class KdeAppletWindow :
  public SigC::Object,
  public AppletWindow
{
public:  
  KdeAppletWindow(AppletControl *control);
  virtual ~KdeAppletWindow();

  void update();
  void fire_kde_applet();

  void on_menu_restbreak_now();
  void button_clicked(int button);

private:
  //! Gtk timerbox viewer
  TimerBoxGtkView *view;
   
  //! The Gtk+ plug in the panel.
  Gtk::Plug *plug;

  //! Container to put the timers in..
  Gtk::Bin *container;
  
  //! The system tray menu.
  Gtk::Menu *tray_menu;

  //! Allign break vertically.
  bool applet_vertical;

  //! Size of the applet
  int applet_size;

#ifdef HAVE_GTKMM24
  Gtk::Requisition last_size;
#else
  GtkRequisition last_size;
#endif

  //!
  AppletControl *control;

  bool applet_active;
  
private:
  void init_applet();
  void cleanup_applet();
  AppletActivateResult activate_applet();
  void deactivate_applet();
    
  static gboolean destroy_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
  
  // Events.
  void on_embedded();
  bool on_button_press_event(GdkEventButton *event);
  bool on_delete_event(GdkEventAny*);
  bool delete_event(GdkEventAny *event);

  bool plug_window(int w);
  bool get_size(int &size);
  bool get_vertical(bool &vertical);
  bool set_size(int width, int height);
};

#endif // KDEAPPLETWINDOW_HH

// AppletWindow.hh --- Main info Window
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
// $Id$
//

#ifndef APPLETWINDOW_HH
#define APPLETWINDOW_HH

#include "preinclude.h"
#include <stdio.h>

#include "ConfiguratorListener.hh"

#ifdef HAVE_GNOME
#include <gnome.h>
#include <bonobo.h>
#include <bonobo/bonobo-xobject.h>
#include "Workrave-Applet.h"
#include "Workrave-Control.h"
#endif

class GUI;
class TimeBar;
class NetworkLogDialog;
class TimerBox;

#include <gtkmm.h>
#include <gtkmm/plug.h>

using namespace std;

class AppletWindow :
  public ConfiguratorListener,
  public SigC::Object
{
public:  
  enum AppletMode { APPLET_DISABLED, APPLET_TRAY, APPLET_GNOME };

  AppletWindow();
  ~AppletWindow();

  void update();
  void fire();

  AppletMode get_applet_mode() const;
  
  void on_menu_restbreak_now();
  void button_clicked(int button);
#ifdef HAVE_GNOME
  void set_menu_active(int menu, bool active);
  bool get_menu_active(int menu);
  void set_applet_vertical(bool vertical);
  void set_applet_size(int size);
  void set_applet_control(GNOME_Workrave_AppletControl applet_control);
#endif

  // ConfiguratorListener
  void config_changed_notify(string key);
  void read_configuration();
  
public:
  static bool is_enabled();
  static void set_enabled(bool enabled);
  static const string CFG_KEY_APPLET_ENABLED;
  
private:
  TimerBox *timers_box;
  
  //! Current applet mode.
  AppletMode mode;

  //! The Gtk+ plug in the panel.
  Gtk::Plug *plug;

  //! Container to put the timers in..
  Gtk::Bin *container;
  
  //! The system tray menu.
  Gtk::Menu *tray_menu;

#ifdef HAVE_GNOME
  // 
  GNOME_Workrave_AppletControl applet_control;
#endif

  //! Retry to initialize the panel again?
  bool retry_init;

  //! Reconfigure the panel.
  bool reconfigure;

  //! Allign break vertically.
  bool applet_vertical;

  //! Size of the applet
  int applet_size;

  //! Applet enabled?
  bool applet_enabled;
  
private:
  void init();
  void init_applet();
  bool init_tray_applet();

  void destroy_applet();
  void destroy_tray_applet();

#ifdef HAVE_GNOME
  bool init_gnome_applet();
  void destroy_gnome_applet();
#endif

  // Events.
  bool on_button_press_event(GdkEventButton *event);
  bool on_delete_event(GdkEventAny*);
  bool delete_event(GdkEventAny *event);
};

#endif // APPLETWINDOW_HH

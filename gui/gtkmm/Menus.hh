// Menus.hh --- Main info Window
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

#ifndef MENUS_HH
#define MENUS_HH

#ifdef HAVE_GNOME
#include <gnome.h>
#endif

class GUI;
class TimeBar;
class NetworkLogDialog;
class MainWindow;
class AppletWindow;

#include <gtkmm.h>

using namespace std;

#define MAX_CHECKMENUS 4

class Menus :
  public SigC::Object
{
public:  
  Menus();
  ~Menus();

  Gtk::Menu *create_menu(Gtk::CheckMenuItem *check_menus[4]);
  Gtk::Menu *create_main_window_menu();
  Gtk::Menu *create_tray_menu();
  
  static Menus *get_instance();

  void set_main_window(MainWindow *main);
#ifdef HAVE_GNOME
  void set_applet_window(AppletWindow *applet);
#endif
  void resync_applet();
  
private:
  void sync_mode_menu(int mode);
  void sync_tray_menu(bool active);
  
#ifdef HAVE_DISTRIBUTION
  void on_menu_network_log_main_window();
  void on_menu_network_log_tray();
  void on_network_log_response(int response);
#endif
  void on_menu_normal_menu(Gtk::CheckMenuItem *);
  void on_menu_suspend_menu(Gtk::CheckMenuItem *);
  void on_menu_quiet_menu(Gtk::CheckMenuItem *);
  
public:  
  // Menu actions.
  void on_menu_open_main_window();
  void on_menu_restbreak_now();
  void on_menu_about();
  void on_menu_quit();
  void on_menu_preferences();
  void on_menu_statistics();
  void on_menu_normal();
  void on_menu_suspend();
  void on_menu_quiet();
#ifndef NDEBUG
  void on_test_me();
#endif
  void on_menu_network_join();
  void on_menu_network_leave();
  void on_menu_network_reconnect();
  void on_menu_network_log(bool active);

protected:
  //! Interface to the GUI.
  GUI *gui;

private:
  //! The one and only instance
  static Menus *instance;
  
#ifdef HAVE_GNOME  
  //! The applet windows
  AppletWindow *applet_window;
#endif

#ifdef HAVE_DISTRIBUTION
  //! The log dialog.
  NetworkLogDialog *network_log_dialog;
#endif
  
  //! The main window.
  MainWindow *main_window;

  //! The system tray popup menu items.
  Gtk::CheckMenuItem *tray_check_menus[MAX_CHECKMENUS];

  //! The popup mode menu items
  Gtk::CheckMenuItem *main_window_check_menus[MAX_CHECKMENUS];
};


inline void
Menus::set_main_window(MainWindow *main)
{
  main_window = main;
}


#ifdef HAVE_GNOME
inline void
Menus::set_applet_window(AppletWindow *applet)
{
  applet_window = applet;
}
#endif


inline Menus *
Menus::get_instance()
{
  if (instance == NULL)
    {
      instance = new Menus();
    }
       
  return instance;
}

#endif // MENUS_HH

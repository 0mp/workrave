// MainWindow.hh --- Main info Window
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

#ifndef MAINWINDOW_HH
#define MAINWINDOW_HH

#define NOMINMAX

#include <stdio.h>

#include "preinclude.h"

class GUI;
class TimeBar;
class NetworkLogDialog;

#include <gtkmm.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "ConfiguratorListener.hh"
#include "TimerWindow.hh"

using namespace std;

class MainWindow :
  public Gtk::Window,
  public ConfiguratorListener,
  public TimerWindow
{
public:  
  MainWindow();
  ~MainWindow();

  void update();
  static bool get_always_on_top();
  static void set_always_on_top(bool b);

protected:
  bool on_button_press_event(GdkEventButton *event);

private:
  //! Connection to the delete_event signal.
  SigC::Connection delete_connection;

  //! Connection to the timeout timer.
  SigC::Connection timer_connection;

  //! Table containing all timer information
  Gtk::Table *timers_box;

  //! The popup menu.
  Gtk::Menu *popup_menu;

  //! Is the monitoring function suspended?
  bool monitor_suspended;

private:
  //
  void init();
  void setup();
  void config_changed_notify(string key);

  // Events.
  bool on_delete_event(GdkEventAny*);

public:  
  static void set_start_in_tray(bool b);
  static bool get_start_in_tray();
  static const string CFG_KEY_MAIN_WINDOW_START_IN_TRAY;
  
#ifdef WIN32
public:
  void win32_remember_position();

  static const string CFG_KEY_MAIN_WINDOW_X;
  static const string CFG_KEY_MAIN_WINDOW_Y;
  
private:
  void win32_show(bool b);
  void win32_init();
  void win32_exit();
  void win32_on_tray_open();
  static void win32_get_start_position(int &x, int &y);
  static void win32_set_start_position(int x, int y);

  static LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT uMsg,
                                            WPARAM wParam, LPARAM lParam);

  Gtk::Menu *win32_tray_menu;
  HWND win32_main_hwnd;
  NOTIFYICONDATA win32_tray_icon;
#endif
};

#endif // MAINWINDOW_HH

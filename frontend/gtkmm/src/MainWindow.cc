// MainWindow.cc --- Main info Window
//
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007 Rob Caelers & Raymond Penners
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

#include "preinclude.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include <gdk/gdkwin32.h>
#include <shellapi.h>
#endif

#include "nls.h"
#include "debug.hh"

#include <unistd.h>
#include <iostream>

#ifdef HAVE_GNOME
#include <gnome.h>
#endif

#include "AppletControl.hh"

#include "TimerBoxGtkView.hh"
#include "TimerBoxControl.hh"
#include "MainWindow.hh"
#include "PreferencesDialog.hh"
#include "StatisticsDialog.hh"
#include "WindowHints.hh"
#include "TimeBar.hh"
#include "GUI.hh"
#include "Util.hh"
#ifdef WIN32
#include "StatusIcon.hh"
#endif
#include "Text.hh"

#include "CoreFactory.hh"
#include "IConfigurator.hh"
#include "ITimer.hh"
#include "IStatistics.hh"

#ifdef HAVE_DISTRIBUTION
#include "NetworkJoinDialog.hh"
#include "NetworkLogDialog.hh"
#endif


#include "Menus.hh"

#include <gtkmm/menu.h>
#include <gtkmm/stock.h>

#include "gtk/gtkmenu.h"


const string MainWindow::CFG_KEY_MAIN_WINDOW = "gui/main_window";
const string MainWindow::CFG_KEY_MAIN_WINDOW_ALWAYS_ON_TOP = "gui/main_window/always_on_top";

const string MainWindow::CFG_KEY_MAIN_WINDOW_START_IN_TRAY
= "gui/main_window/start_in_tray";

#ifdef WIN32
const char *WIN32_MAIN_CLASS_NAME = "Workrave";
const UINT MYWM_TRAY_MESSAGE = WM_USER +0x100;
#endif

const string MainWindow::CFG_KEY_MAIN_WINDOW_X
= "gui/main_window/x";
const string MainWindow::CFG_KEY_MAIN_WINDOW_Y
= "gui/main_window/y";
const string MainWindow::CFG_KEY_MAIN_WINDOW_HEAD
= "gui/main_window/head";



//! Constructor.
/*!
 *  \param gui the main GUI entry point.
 *  \param control Interface to the controller.
 */
MainWindow::MainWindow() :
  enabled(true),
  timer_box_control(NULL),
  timer_box_view(NULL),
  monitor_suspended(false),
  visible(true),
  applet_active(false),
  window_location(-1, -1),
  window_head_location(-1, -1),
  window_relocated_location(-1, -1)
{
#ifdef HAVE_X
  leader = NULL;
#endif
  init();
}


//! Destructor.
MainWindow::~MainWindow()
{
  TRACE_ENTER("MainWindow::~MainWindow");
  delete timer_box_control;
#ifdef WIN32
  win32_exit();
#endif
#ifdef HAVE_X
  delete leader;
#endif

  TRACE_EXIT();
}

//! Initializes the main window.
void
MainWindow::init()
{
  TRACE_ENTER("MainWindow::init");

  set_border_width(2);
  set_resizable(false);

  list<Glib::RefPtr<Gdk::Pixbuf> > icons;

  char *icon_files[] = { "workrave-icon-small.png",
                         "workrave-icon-medium.png",
                         "workrave-icon-large.png" };

  for (unsigned int i = 0; i < sizeof(icon_files) / sizeof(char *); i++)
    {
      string file = Util::complete_directory(icon_files[i], Util::SEARCH_PATH_IMAGES);

      try
        {
          Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(file);
          icons.push_back(pixbuf);
        }
      catch (...)
        {
        }
    }

  Glib::ListHandle<Glib::RefPtr<Gdk::Pixbuf> > icon_list(icons);
  Gtk::Window::set_default_icon_list(icon_list);

  enabled = TimerBoxControl::is_enabled("main_window");

  Menus *menus = Menus::get_instance();
  menus->set_main_window(this);
  menus->create_menu(Menus::MENU_MAINWINDOW);

  timer_box_view = manage(new TimerBoxGtkView());
  timer_box_control = new TimerBoxControl("main_window", *timer_box_view);
  timer_box_view->set_geometry(ORIENTATION_LEFT, -1);
  timer_box_control->update();
  add(*timer_box_view);

  set_events(get_events() | Gdk::BUTTON_PRESS_MASK | Gdk::SUBSTRUCTURE_MASK);

  // Necessary for popup menu
  realize_if_needed();

  Glib::RefPtr<Gdk::Window> window = get_window();

  // Window decorators
  window->set_decorations(Gdk::DECOR_BORDER
                          |Gdk::DECOR_TITLE
                          |Gdk::DECOR_MENU);
  // This used to be W32 only:
  //   window->set_functions(Gdk::FUNC_CLOSE|Gdk::FUNC_MOVE);

  // (end window decorators)

#ifdef HAVE_X
  // HACK. this sets a different group leader in the WM_HINTS....
  // Without this hack, metacity makes ALL windows on-top.
  leader = new Gtk::Window(Gtk::WINDOW_POPUP);
  gtk_widget_realize(GTK_WIDGET(leader->gobj()));
  Glib::RefPtr<Gdk::Window> leader_window = leader->get_window();
  window->set_group(leader_window);
#endif

  stick();
  setup();

#ifdef WIN32

  win32_init();
  set_gravity(Gdk::GRAVITY_STATIC);
  set_position(Gtk::WIN_POS_NONE);

#ifndef HAVE_NOT_PROPER_SIZED_MAIN_WINDOW_ON_STARTUP
  // This is the proper code, see hacked code below.
  if (!enabled)
    {
      move(-1024, 0);
      show_all();
      win32_show(false);
      move_to_start_position();
    }
  else
    {
      move_to_start_position();;
      show_all();
    }
#else // Hack deprecated: Since GTK+ 2.10 no longer necessary

  // Hack: since GTK+ 2.2.4 the window is too wide, it incorporates room
  // for the "_ [ ] [X]" buttons somehow. This hack fixes just that.
  move(-1024, 0); // Move off-screen to reduce wide->narrow flickering
  show_all();
  HWND hwnd = (HWND) GDK_WINDOW_HWND(window->gobj());
  SetWindowPos(hwnd, NULL, 0, 0, 1, 1,
               SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE
               |SWP_NOOWNERZORDER|SWP_NOMOVE);
  if (! enabled)
    {
      win32_show(false);
    }
  move_to_start_position();
  // (end of hack)
#endif

#else
  set_gravity(Gdk::GRAVITY_STATIC);
  set_position(Gtk::WIN_POS_NONE);
  show_all();
  move_to_start_position();

  if (!enabled) //  || get_start_in_tray())
    {
      iconify();
      close_window();
    }
#endif
  setup();
  set_title("Workrave");

  IConfigurator *config = CoreFactory::get_configurator();
  config->add_listener(TimerBoxControl::CFG_KEY_TIMERBOX + "main_window", this);


  TRACE_EXIT();
}




//! Setup configuration settings.
void
MainWindow::setup()
{
  TRACE_ENTER("MainWindow::setup");

  bool always_on_top = get_always_on_top();
  WindowHints::set_always_on_top(Gtk::Widget::gobj(), always_on_top);

  bool new_enabled = TimerBoxControl::is_enabled("main_window");

  TRACE_MSG("on top " << always_on_top);
  TRACE_MSG("enabled " << new_enabled);

  if (enabled != new_enabled)
    {
      enabled = new_enabled;
      if (enabled)
        {
          open_window();
        }
      else
        {
          close_window();
        }
    }
  if (always_on_top)
    {
      raise();
    }

  TRACE_EXIT()
}


//! Updates the main window.
void
MainWindow::update()
{
  timer_box_control->update();
}


void MainWindow::on_activate()
{
  open_window();
}



//! Opens the main window.
void
MainWindow::open_window()
{
  TRACE_ENTER("MainWindow::open_window");
  if (timer_box_view->get_visible_count() > 0)
    {
#ifdef WIN32
      win32_show(true);
#else
      show_all();
      deiconify();
#endif

      int x, y, head;
      set_position(Gtk::WIN_POS_NONE);
      set_gravity(Gdk::GRAVITY_STATIC);
      get_start_position(x, y, head);

      GtkRequisition req;
      on_size_request(&req);
      GUI::get_instance()->bound_head(x, y, req.width, req.height, head);

      GUI::get_instance()->map_from_head(x, y, head);
      TRACE_MSG("moving to " << x << " " << y);
      move(x, y);

    }
  TRACE_EXIT();
}



//! Closes the main window.
void
MainWindow::close_window()
{
#ifdef WIN32
  win32_show(false);
#else
  if (applet_active)
    {
      hide_all();
    }
  else
    {
      iconify();
    }
#endif
}


//! Toggles the main window.
void
MainWindow::toggle_window()
{
  TimerBoxControl::set_enabled("main_window", !enabled);
}


//! User has closed the main window.
bool
MainWindow::on_delete_event(GdkEventAny *)
{
  TRACE_ENTER("MainWindow::on_delete_event");

#ifdef WIN32
  win32_show(false);
#else
  GUI *gui = GUI::get_instance();
  assert(gui != NULL);

#if defined(HAVE_GNOME) || defined(HAVE_KDE)
  bool terminate = true;
  AppletControl *applet_control = gui->get_applet_control();
  if (applet_control != NULL)
    {
      terminate = !applet_control->is_visible();
    }

  if (terminate)
    {
      gui->terminate();
    }
  else
    {
      close_window();
      TimerBoxControl::set_enabled("main_window", false);
    }
#else
  gui->terminate();
#endif // HAVE_GNOME || HAVE_KDE
#endif // WIN32

  TRACE_EXIT();
  return true;
}



//! Users pressed some mouse button in the main window.
bool
MainWindow::on_button_press_event(GdkEventButton *event)
{
  TRACE_ENTER("MainWindow::on_button_press_event");
  bool ret = false;

  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
    {
      Menus::get_instance()->popup(Menus::MENU_MAINWINDOW,
                                   event->button, event->time);
      ret = true;
    }

  TRACE_EXIT();
  return ret;
}



bool
MainWindow::on_window_state_event(GdkEventWindowState *event)
{
  TRACE_ENTER("MainWindow::on_window_state_event");
  if (event != NULL)
    {
      if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
        {
          bool iconified = event->new_window_state & GDK_WINDOW_STATE_ICONIFIED;
          TimerBoxControl::set_enabled("main_window", !iconified);
        }
    }

  TRACE_EXIT();
  return true;
}


void
MainWindow::config_changed_notify(string key)
{
  TRACE_ENTER_MSG("MainWindow::config_changed_notify", key);
  if (key != CFG_KEY_MAIN_WINDOW_HEAD
      && key != CFG_KEY_MAIN_WINDOW_X
      && key != CFG_KEY_MAIN_WINDOW_Y)
    {
      setup();
    }
  TRACE_EXIT();
}

bool
MainWindow::get_always_on_top()
{
  bool rc;
  CoreFactory::get_configurator()
    ->get_value_default(MainWindow::CFG_KEY_MAIN_WINDOW_ALWAYS_ON_TOP,
                        &rc,
                        false);
  return rc;
}


void
MainWindow::set_always_on_top(bool b)
{
  CoreFactory::get_configurator()
    ->set_value(MainWindow::CFG_KEY_MAIN_WINDOW_ALWAYS_ON_TOP, b);
}


bool
MainWindow::get_start_in_tray()
{
  bool rc;
  CoreFactory::get_configurator()
    ->get_value_default(CFG_KEY_MAIN_WINDOW_START_IN_TRAY, &rc, false);
  return rc;
}


void
MainWindow::set_start_in_tray(bool b)
{
  CoreFactory::get_configurator()
    ->set_value(CFG_KEY_MAIN_WINDOW_START_IN_TRAY, b);
}


#ifdef WIN32
void
MainWindow::win32_show(bool b)
{
  // Gtk's hide() seems to quit the program.
  GtkWidget *window = Gtk::Widget::gobj();
  GdkWindow *gdk_window = window->window;
  HWND hwnd = (HWND) GDK_WINDOW_HWND(gdk_window);
  ShowWindow(hwnd, b ? SW_SHOWNORMAL : SW_HIDE);

  if( b )
    // jay satiro, workrave project, june 2007
    // redistribute under GNU terms.
    {
      SetWindowPos( hwnd, 0, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );
      {
        HWND hFore = GetForegroundWindow();
        DWORD dThisThread, dForeThread, dForePID;
  
        dThisThread = GetCurrentThreadId();
        dForeThread = GetWindowThreadProcessId( hFore, &dForePID );
    
        AttachThreadInput( dThisThread, dForeThread, TRUE );
        /**/
        present();
        /**/
        hFore = GetForegroundWindow();
        if( hwnd != hFore )
          {
            BringWindowToTop( hwnd );
            SetForegroundWindow( hwnd );
          }
        AttachThreadInput( dThisThread, dForeThread, FALSE );
      }
    }
  else
    {
      ShowWindow( hwnd, SW_HIDE );
    }
}


void
MainWindow::win32_init()
{
  TRACE_ENTER("MainWindow::win32_init");

  win32_hinstance = (HINSTANCE) GetModuleHandle(NULL);

  WNDCLASSEX wclass =
    {
      sizeof(WNDCLASSEX),
      0,
      win32_window_proc,
      0,
      0,
      win32_hinstance,
      NULL,
      NULL,
      NULL,
      NULL,
      WIN32_MAIN_CLASS_NAME,
      NULL
    };
  /* ATOM atom = */ RegisterClassEx(&wclass);

  win32_main_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                                   WIN32_MAIN_CLASS_NAME,
                                   "Workrave",
                                   WS_OVERLAPPED,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   (HWND)NULL,
                                   (HMENU)NULL,
                                   win32_hinstance,
                                   (LPSTR)NULL);
  ShowWindow(win32_main_hwnd, SW_HIDE);

  // User data
  SetWindowLong(win32_main_hwnd, GWL_USERDATA, (LONG) this);

  // Reassign ownership
  GtkWidget *window = Gtk::Widget::gobj();
  GdkWindow *gdk_window = window->window;
  HWND hwnd = (HWND) GDK_WINDOW_HWND(gdk_window);
  SetWindowLong(hwnd, GWL_HWNDPARENT, (LONG) win32_main_hwnd);

  TRACE_EXIT();
}

void
MainWindow::win32_exit()
{
  DestroyWindow(win32_main_hwnd);
  UnregisterClass(WIN32_MAIN_CLASS_NAME, GetModuleHandle(NULL));
}

#endif

void
MainWindow::get_start_position(int &x, int &y, int &head)
{
  TRACE_ENTER("MainWindow::get_start_position");
  // FIXME: Default to right-bottom instead of 256x256
  IConfigurator *cfg = CoreFactory::get_configurator();
  cfg->get_value_default(CFG_KEY_MAIN_WINDOW_X, &x, 256);
  cfg->get_value_default(CFG_KEY_MAIN_WINDOW_Y, &y, 256);
  cfg->get_value_default(CFG_KEY_MAIN_WINDOW_HEAD, &head, 0);
  if (head < 0)
    {
      head = 0;
    }
  TRACE_MSG(x << " " << y << " " << head);
  TRACE_EXIT();
}


void
MainWindow::set_start_position(int x, int y, int head)
{
  TRACE_ENTER_MSG("MainWindow::set_start_position",
                  x << " " << y << " " << head);
  IConfigurator *cfg = CoreFactory::get_configurator();
  cfg->set_value(CFG_KEY_MAIN_WINDOW_X, x);
  cfg->set_value(CFG_KEY_MAIN_WINDOW_Y, y);
  cfg->set_value(CFG_KEY_MAIN_WINDOW_HEAD, head);
  TRACE_EXIT();
}



void
MainWindow::move_to_start_position()
{
  TRACE_ENTER("MainWindow::move_to_start_position");

  int x, y, head;
  get_start_position(x, y, head);

  GtkRequisition req;
  on_size_request(&req);

  GUI::get_instance()->bound_head(x, y, req.width, req.height, head);

  window_head_location.set_x(x);
  window_head_location.set_y(y);

  GUI::get_instance()->map_from_head(x, y, head);

  TRACE_MSG("Main window size " << req.width << " " << req.height);

  window_location.set_x(x);
  window_location.set_y(y);
  window_relocated_location.set_x(x);
  window_relocated_location.set_y(y);
  TRACE_MSG("moving to " << x << " " << y);

  move(x, y);
}


void
MainWindow::set_applet_active(bool a)
{
  TRACE_ENTER_MSG("MainWindow::set_applet_active", a);
  if (applet_active != a)
    {
      applet_active = a;

      if (!enabled)
        {
          if (applet_active)
            {
              hide_all();
            }
          else
            {
              iconify();
              show_all();
            }
        }
    }
  TRACE_EXIT();
}


bool
MainWindow::on_configure_event(GdkEventConfigure *event)
{
  TRACE_ENTER_MSG("MainWindow::on_configure_event",
                  event->x << " " << event->y);
  locate_window(event);
  bool ret =  Widget::on_configure_event(event);
  TRACE_EXIT();
  return ret;
}

void
MainWindow::locate_window(GdkEventConfigure *event)
{
  TRACE_ENTER("MainWindow::locate_window");
  int x, y;
  int width, height;

  (void) event;

#ifndef WIN32
  // Returns bogus results on windows...sometime.
  if (event != NULL)
    {
      x = event->x;
      y = event->y;
      width = event->width;
      height = event->height;

      TRACE_MSG("main window2 = " << x << " " << y);
    }
  else
#endif
    {
      get_position(x, y);

      GtkRequisition req;
      on_size_request(&req);
      width = req.width;
      height = req.height;

      TRACE_MSG("main window1 = " << x << " " << y);
    }


  TRACE_MSG("main window = " << x << " " << y);

  if (x <= 0 && y <= 0)
    {
      // FIXME: this is a hack...
      TRACE_EXIT();
      return;
    }

  if (x != window_relocated_location.get_x() ||
      y != window_relocated_location.get_y())
    {
      window_location.set_x(x);
      window_location.set_y(y);

      int head = GUI::get_instance()->map_to_head(x, y);
      TRACE_MSG("main window head = " << x << " " << y);
      // Stores location relative to origin of current head.

      bool rc = GUI::get_instance()->bound_head(x, y, width, height, head);

      window_head_location.set_x(x);
      window_head_location.set_y(y);
      set_start_position(x, y, head);

      if (rc)
        {
          move_to_start_position();
        }
    }
  TRACE_EXIT();
}


void
MainWindow::relocate_window(int width, int height)
{
  TRACE_ENTER_MSG("MainWindow::relocate_window", width << " " << height);
  int x = window_location.get_x();
  int y = window_location.get_y();

  if (x <= 0 || y <= 0)
    {
      TRACE_MSG("invalid " << x << " " << y);
    }
  else if (x <= width && y <= height)
    {
      TRACE_MSG(x << " " << y);
      TRACE_MSG("fits, moving to");
      set_position(Gtk::WIN_POS_NONE);
      move(x, y);
    }
  else
    {
      TRACE_MSG("move to differt head");
      x = window_head_location.get_x();
      y = window_head_location.get_y();

      GUI *gui = GUI::get_instance();
      int num_heads = gui->get_number_of_heads();
      for (int i = 0; i < num_heads; i++)
        {
          HeadInfo &head = gui->get_head(i);
          if (head.valid)
            {
              GtkRequisition req;
              on_size_request(&req);
              GUI::get_instance()->bound_head(x, y, req.width, req.height, i);

              gui->map_from_head(x, y, i);
              break;
            }
        }

      if (x < 0)
        {
          x = 0;
        }
      if (y < 0)
        {
          y = 0;
        }

      TRACE_MSG("moving to " << x << " " << y);
      window_relocated_location.set_x(x);
      window_relocated_location.set_y(y);

      set_position(Gtk::WIN_POS_NONE);
      move(x, y);
    }

  TRACE_EXIT();
}

#ifdef WIN32

LRESULT CALLBACK
MainWindow::win32_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam)
{
  TRACE_ENTER("MainWindow::win32_window_proc");
  TRACE_EXIT();
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

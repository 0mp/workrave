// BreakWindow.cc --- base class for the break windows
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
#include "debug.hh"
#include "nls.h"

#include <gtkmm/window.h>
#include <gtkmm/stock.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/button.h>

#include <math.h>

#include "BreakWindow.hh"
#include "BreakResponseInterface.hh"
#include "GtkUtil.hh"
#include "WindowHints.hh"
#include "Frame.hh"
#include "System.hh"
#include "Util.hh"


//! Constructor
/*!
 *  \param control The controller.
 */
BreakWindow::BreakWindow(BreakId break_id, HeadInfo &head,
                         bool ignorable, GUI::BlockMode mode) :
         Gtk::Window(mode==GUI::BLOCK_MODE_NONE
                     ? Gtk::WINDOW_TOPLEVEL
                     : Gtk::WINDOW_POPUP),
         block_mode(mode),
         ignorable_break(ignorable),
         break_response(NULL),
         gui(NULL)
{
  this->break_id = break_id;
  
#ifdef HAVE_X
  GtkUtil::set_wmclass(*this, "Break");
#endif

  // On W32, must be *before* realize, otherwise a border is drawn.
  set_resizable(false);
  
  // Need to realize window before it is shown
  // Otherwise, there is not gobj()...
  realize();
  if (mode == GUI::BLOCK_MODE_NONE)
    {
      Glib::RefPtr<Gdk::Window> window = get_window();
      window->set_functions(Gdk::FUNC_MOVE);
    }

  
  this->head = head;
#ifdef HAVE_GTK_MULTIHEAD  
  Gtk::Window::set_screen(head.screen);
#endif
}

//! Init GUI
void
BreakWindow::init_gui()
{
  if (gui == NULL)
    {
      gui = manage(create_gui());

      if (block_mode == GUI::BLOCK_MODE_NONE)
        {
          set_border_width(12);
          add(*gui);
        }
      else
        {
          set_border_width(0);
          Frame *window_frame = manage(new Frame());
          window_frame->set_border_width(12);
          window_frame->set_frame_style(Frame::STYLE_BREAK_WINDOW);
          window_frame->add(*gui);
          if (block_mode == GUI::BLOCK_MODE_ALL)
            {
              set_size_request(head.get_width(),
                               head.get_height());
              set_app_paintable(true);
              set_background_pixmap();
              Gtk::Alignment *align
                = manage(new Gtk::Alignment(0.5, 0.5, 0.0, 0.0));
              align->add(*window_frame);
              add(*align);
            }
          else
            {
              add(*window_frame);
            }
        }
      show_all_children();
      stick();
  
      // Set window hints.
      WindowHints::set_skip_winlist(Gtk::Widget::gobj(), true);
      WindowHints::set_always_on_top(Gtk::Widget::gobj(), true);

      // FIXME: check if it was intentionally not unset for RB
      if (break_id != BREAK_ID_REST_BREAK)
        {
          unset_flags(Gtk::CAN_FOCUS);
        }
    }
}

//! Courtesy of DrWright
static GdkPixbuf *
create_tile_pixbuf (GdkPixbuf    *dest_pixbuf,
                    GdkPixbuf    *src_pixbuf,
                    GdkRectangle *field_geom,
                    guint         alpha,
                    GdkColor     *bg_color) 
{
  gboolean need_composite;
  gboolean use_simple;
  int  cx, cy;
  int  colorv;
  gint     pwidth, pheight;

  need_composite = (alpha < 255 || gdk_pixbuf_get_has_alpha (src_pixbuf));
  use_simple = (dest_pixbuf == NULL);

  if (dest_pixbuf == NULL)
    dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, field_geom->width, field_geom->height);

  if (need_composite && use_simple)
    colorv = ((bg_color->red & 0xff00) << 8) |
      (bg_color->green & 0xff00) |
      ((bg_color->blue & 0xff00) >> 8);
  else
    colorv = 0;

  pwidth = gdk_pixbuf_get_width (src_pixbuf);
  pheight = gdk_pixbuf_get_height (src_pixbuf);

  for (cy = 0; cy < field_geom->height; cy += pheight) {
    for (cx = 0; cx < field_geom->width; cx += pwidth) {
      if (need_composite && !use_simple)
        gdk_pixbuf_composite
          (src_pixbuf, dest_pixbuf,
           cx, cy,
           MIN (pwidth, field_geom->width - cx), 
           MIN (pheight, field_geom->height - cy),
           cx, cy,
           1.0, 1.0,
           GDK_INTERP_NEAREST,
           alpha);
      else if (need_composite && use_simple)
        gdk_pixbuf_composite_color
          (src_pixbuf, dest_pixbuf,
           cx, cy,
           MIN (pwidth, field_geom->width - cx), 
           MIN (pheight, field_geom->height - cy),
           cx, cy,
           1.0, 1.0,
           GDK_INTERP_NEAREST,
           alpha,
           65536, 65536, 65536,
           colorv, colorv);
      else
        gdk_pixbuf_copy_area
          (src_pixbuf,
           0, 0,
           MIN (pwidth, field_geom->width - cx),
           MIN (pheight, field_geom->height - cy),
           dest_pixbuf,
           cx, cy);
    }
  }

  return dest_pixbuf;
}

//! Courtesy of DrWright
void
BreakWindow::set_background_pixmap()
{
  GdkPixbuf          *tmp_pixbuf, *pixbuf, *tile_pixbuf;
  GdkPixmap          *pixmap;
  GdkRectangle        rect;
  GdkColor            color;
  int screen_x, screen_y, screen_width, screen_height;

  screen_x = head.get_x();
  screen_y = head.get_y();
  screen_width = head.get_width();
  screen_height = head.get_height();

  tmp_pixbuf = gdk_pixbuf_get_from_drawable (NULL,
                                             gdk_get_default_root_window (),
                                             gdk_colormap_get_system (),
                                             screen_x,
                                             screen_y,
                                             0,
                                             0,
                                             screen_width,
                                             screen_height);

  std::string file = Util::complete_directory
    ("ocean-stripes.png", Util::SEARCH_PATH_IMAGES);
  pixbuf = gdk_pixbuf_new_from_file (file.c_str(), NULL);
  
  rect.x = 0;
  rect.y = 0;
  rect.width = screen_width;
  rect.height = screen_height;

  color.red = 0;
  color.blue = 0;
  color.green = 0;
	
  tile_pixbuf = create_tile_pixbuf (NULL,
                                    pixbuf,
                                    &rect,
                                    155,
                                    &color);

  g_object_unref (pixbuf);

  gdk_pixbuf_composite (tile_pixbuf,
                        tmp_pixbuf,
                        0,
                        0,
                        screen_width,
                        screen_height,
                        0,
                        0,
                        1,
                        1,
                        GDK_INTERP_NEAREST,
                        225);

  g_object_unref (tile_pixbuf);

  pixmap = gdk_pixmap_new (GTK_WIDGET (gobj())->window,
                           screen_width,
                           screen_height,
                           -1);

  gdk_pixbuf_render_to_drawable_alpha (tmp_pixbuf,
                                       pixmap,
                                       0,
                                       0,
                                       0,
                                       0,
                                       screen_width,
                                       screen_height,
                                       GDK_PIXBUF_ALPHA_BILEVEL,
                                       0,
                                       GDK_RGB_DITHER_NONE,
                                       0,
                                       0);
  g_object_unref (tmp_pixbuf);

  gdk_window_set_back_pixmap (GTK_WIDGET (gobj())->window, pixmap, FALSE);
  g_object_unref (pixmap);
}

        
//! Destructor.
BreakWindow::~BreakWindow()
{
  TRACE_ENTER("BreakWindow::~BreakWindow");
  TRACE_EXIT();
}


//! Centers the window.
void
BreakWindow::center()
{
  GtkUtil::center_window(*this, head);
}



//! Creates the lock button
Gtk::Button *
BreakWindow::create_lock_button()
{
  Gtk::Button *ret;
  if (System::is_lockable())
    {
      ret = GtkUtil::create_image_button(_("Lock"), "lock.png");
      ret->signal_clicked()
        .connect(SigC::slot(*this, &BreakWindow::on_lock_button_clicked));
      GTK_WIDGET_UNSET_FLAGS(ret->gobj(), GTK_CAN_FOCUS);
    }
  else
    {
      ret = NULL;
    }
  return ret;
}

//! Creates the lock button
Gtk::Button *
BreakWindow::create_shutdown_button()
{
  Gtk::Button *ret;
  if (System::is_shutdown_supported())
    {
      ret = GtkUtil::create_image_button(_("Shut down"), "shutdown.png");
      ret->signal_clicked()
        .connect(SigC::slot(*this, &BreakWindow::on_shutdown_button_clicked));
      GTK_WIDGET_UNSET_FLAGS(ret->gobj(), GTK_CAN_FOCUS);
    }
  else
    {
      ret = NULL;
    }
  return ret;
}

//! Creates the skip button.
Gtk::Button *
BreakWindow::create_skip_button()
{
  Gtk::Button *ret;
  ret = GtkUtil::create_custom_stock_button(_("Skip"), Gtk::Stock::CLOSE);
  ret->signal_clicked()
    .connect(SigC::slot(*this, &BreakWindow::on_skip_button_clicked));
  GTK_WIDGET_UNSET_FLAGS(ret->gobj(), GTK_CAN_FOCUS);
  return ret;
}


//! Creates the postpone button.
Gtk::Button *
BreakWindow::create_postpone_button()
{
  Gtk::Button *ret;
  ret = GtkUtil::create_custom_stock_button(_("Postpone"), Gtk::Stock::REDO);
  ret->signal_clicked()
    .connect(SigC::slot(*this, &BreakWindow::on_postpone_button_clicked));
  GTK_WIDGET_UNSET_FLAGS(ret->gobj(), GTK_CAN_FOCUS);
  return ret;
}


//! The lock button was clicked.
void
BreakWindow::on_lock_button_clicked()
{
  System::lock();
}

//! The lock button was clicked.
void
BreakWindow::on_shutdown_button_clicked()
{
  System::shutdown();
}

//! User has closed the main window.
bool
BreakWindow::on_delete_event(GdkEventAny *)
{
  on_postpone_button_clicked();
  return TRUE;
}

//! Break response
inline void
BreakWindow::set_response(BreakResponseInterface *bri)
{
  break_response = bri;
}

//! The postpone button was clicked.
void
BreakWindow::on_postpone_button_clicked()
{
  if (break_response != NULL)
    {
      break_response->postpone_break(break_id);
    }
}



//! The skip button was clicked.
void
BreakWindow::on_skip_button_clicked()
{
  if (break_response != NULL)
    {
      break_response->skip_break(break_id);
    }
}


//! Control buttons.
Gtk::HButtonBox *
BreakWindow::create_break_buttons(bool lockable,
                                  bool shutdownable)
{
  Gtk::HButtonBox *box = NULL;

  if (ignorable_break || lockable || shutdownable)
    {
      box = new Gtk::HButtonBox(Gtk::BUTTONBOX_END, 6);

      Gtk::Button *shutdown_button = NULL;
      if (shutdownable)
        {
          shutdown_button = manage(create_shutdown_button());
        }
      if (shutdown_button != NULL)
        {
          box->pack_end(*shutdown_button, Gtk::SHRINK, 0);
        }
      else if (lockable)
        {
          Gtk::Button *lock_button = manage(create_lock_button());
          if (lock_button != NULL)
            {
              box->pack_end(*lock_button, Gtk::SHRINK, 0);
            }
        }

      if (ignorable_break)
        {
          Gtk::Button *skip_button = manage(create_skip_button());
          box->pack_end(*skip_button, Gtk::SHRINK, 0);

          Gtk::Button *postpone_button = manage(create_postpone_button());
          box->pack_end(*postpone_button, Gtk::SHRINK, 0);
        }
    }

  return box;
}


//! Starts the daily limit.
void
BreakWindow::start()
{
  TRACE_ENTER("BreakWindow::start");

  init_gui();
  center();
  show_all();

#ifdef CAUSES_FVWM_FOCUS_PROBLEMS
  present(); // After grab() please (Windows)
#endif

  TRACE_EXIT();
}

//! Stops the daily limit.
void
BreakWindow::stop()
{
  TRACE_ENTER("BreakWindow::stop");

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
BreakWindow::destroy()
{
  delete this;
}

//! Refresh
void
BreakWindow::refresh()
{
}

Glib::RefPtr<Gdk::Window>
BreakWindow::get_gdk_window()
{
  return get_window();
}


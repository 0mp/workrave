// TimeBar.cc --- Time Bar
//
// Copyright (C) 2002, 2003 Rob Caelers & Raymond Penners
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

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// TODO: must be somewhere else.
#undef THREAD_PRIORITY_NORMAL
#undef DELETE
#undef OK
#undef ERROR

#include "TimeBar.hh"
#include "Text.hh"

const int MARGINX = 4;
const int MARGINY = 3;
const int MIN_HORIZONTAL_BAR_HEIGHT = 20; // stolen from gtk's progress bar

Gdk::Color TimeBar::bar_colors[TimeBar::COLOR_ID_SIZEOF] =
  {
    Gdk::Color("lightblue"),
    Gdk::Color("lightgreen"),
    Gdk::Color("orange"),
    Gdk::Color("#00d4b2"),
    Gdk::Color("lightgreen"),
  };




//! Constructor
TimeBar::TimeBar() :
  bar_value(0),
  bar_max_value(0),
  secondary_bar_value(0),
  secondary_bar_max_value(0),
  bar_text_align(0)
{
  add_events(Gdk::EXPOSURE_MASK);
}


//! Destructor
TimeBar::~TimeBar()
{
}


void TimeBar::on_realize()
{
  // We need to call the base on_realize()
  Gtk::DrawingArea::on_realize();
  // Now we can allocate any additional resources we need
  Glib::RefPtr<Gdk::Window> window = get_window();
  window_gc = Gdk::GC::create(window);

  set_bar_color(COLOR_ID_INACTIVE);
  set_secondary_bar_color(COLOR_ID_INACTIVE);
  set_text_color(Gdk::Color("black"));  //

  Glib::RefPtr<Gtk::Style> style = get_style();
  Gdk::Color light = style->get_light(Gtk::STATE_NORMAL);
  Gdk::Color bg = style->get_bg(Gtk::STATE_NORMAL);
  Gdk::Color mix;
  mix.set_red((light.get_red() + bg.get_red())/2);
  mix.set_blue((light.get_blue() + bg.get_blue())/2);
  mix.set_green((light.get_green() + bg.get_green())/2);
  bar_colors[COLOR_ID_BG] = mix;

#if 1 // FIXME: bug66
  Glib::RefPtr<Gdk::Colormap> colormap = get_colormap();
  for (int i = 0; i < COLOR_ID_SIZEOF; i++)
    {
      colormap->alloc_color(bar_colors[i]);
    }
#endif
  window->clear();
}


void 
TimeBar::on_size_request(GtkRequisition *requisition)
{
  Glib::RefPtr<Pango::Layout> pl = create_pango_layout(bar_text);

  string min_string = Text::time_to_string(-(59+59*60+9*60*60));;
  Glib::RefPtr<Pango::Layout> plmin = create_pango_layout(min_string);

  int width, height;
  pl->get_pixel_size(width, height);

  int mwidth, mheight;
  plmin->get_pixel_size(mwidth, mheight);
  if (mwidth > width)
    width = mwidth;
  if (mheight > height)
    height = mheight;

  requisition->width = width + 2 * MARGINX;
  requisition->height = max(height + 2 * MARGINY, MIN_HORIZONTAL_BAR_HEIGHT);
}


bool
TimeBar::on_expose_event(GdkEventExpose *e)
{
  (void) e;
  int border_size = 2;
  
  // we need a ref to the gdkmm window
  Glib::RefPtr<Gdk::Window> window = get_window();

  // window geometry: x, y, width, height, depth
  int winx, winy, winw, winh, wind;
  window->get_geometry(winx, winy, winw, winh, wind);

  // Border
  Gdk::Rectangle area(&e->area);
  Glib::RefPtr<Gtk::Style> style = get_style();

  window_gc->set_foreground(bar_colors[COLOR_ID_BG]);
  style->paint_shadow(window, Gtk::STATE_NORMAL, Gtk::SHADOW_IN, area,
                      *this, "", 0, 0, winw, winh);
  window->draw_rectangle(window_gc,
                         true, e->area.x+border_size, e->area.y+border_size,
                         e->area.width -2*border_size,
                         e->area.height -2*border_size);

  // Bar
  int bar_width = 0;
  if (bar_max_value > 0)
    {
      bar_width = (bar_value * (winw - 2 * border_size)) / bar_max_value;
    }
  
  // Secondary bar
  int sbar_width = 0;
  if (secondary_bar_max_value >  0)
    {
      sbar_width = (secondary_bar_value * (winw - 2 * border_size)) / secondary_bar_max_value;
    }

  int bar_h = winh - 2 * border_size;
  
  if (sbar_width > 0)
    {
      // Overlap
      assert(secondary_bar_color == COLOR_ID_INACTIVE);
      ColorId overlap_color;
      switch (bar_color)
        {
        case COLOR_ID_ACTIVE:
          overlap_color = COLOR_ID_INACTIVE_OVER_ACTIVE;
          break;
        case COLOR_ID_OVERDUE:
          overlap_color = COLOR_ID_INACTIVE_OVER_OVERDUE;
          break;
        default:
          abort();
        }

      if (sbar_width >= bar_width)
        {
          if (bar_width)
            {
              window_gc->set_foreground(bar_colors[overlap_color]);
              window->draw_rectangle(window_gc, true, border_size, border_size,
                                     bar_width, bar_h);
            }
          if (sbar_width > bar_width)
            {
              window_gc->set_foreground(bar_colors[secondary_bar_color]);
              window->draw_rectangle(window_gc, true,
                                     border_size + bar_width, border_size,
                                     sbar_width - bar_width, bar_h);
            }
        }
      else
        {
          if (sbar_width)
            {
              window_gc->set_foreground(bar_colors[overlap_color]);
              window->draw_rectangle(window_gc, true, border_size, border_size,
                                     sbar_width, bar_h);
            }
          window_gc->set_foreground(bar_colors[bar_color]);
          window->draw_rectangle(window_gc, true,
                                 border_size + sbar_width, border_size,
                                 bar_width - sbar_width, bar_h);
        }
    }
  else
    {
      // No overlap
      window_gc->set_foreground(bar_colors[bar_color]);
      window->draw_rectangle(window_gc, true, border_size, border_size,
                             bar_width, bar_h);
    }
      
  


  // Text
  window_gc->set_foreground(bar_text_color);
  Glib::RefPtr<Pango::Layout> pl1 = create_pango_layout(bar_text);
  int width, height;
  pl1->get_pixel_size(width, height);

  int x;
  if (bar_text_align > 0)
    x = (winw - width - MARGINX);
  else if (bar_text_align < 0)
    x = MARGINX;
  else
    x = (winw - width) / 2;
  window->draw_layout(window_gc, x, (winh - height) / 2, pl1);
  return true;
}


//! Sets the time to be displayed.
void
TimeBar::set_progress(int value, int max_value)
{
  if (value > max_value)
    {
      value = max_value;
    }
  
  bar_value = value;
  bar_max_value = max_value;
}


//! Sets the time to be displayed.
void
TimeBar::set_secondary_progress(int value, int max_value)
{
  if (value > max_value)
    {
      value = max_value;
    }
  
  secondary_bar_value = value;
  secondary_bar_max_value = max_value;
}

//! Sets the text to be displayed.
void
TimeBar::set_text(string text)
{
  bar_text = text;
}


//! Sets text alignment
void
TimeBar::set_text_alignment(int align)
{
  bar_text_align = align;
}


//! Sets the color of the bar.
void
TimeBar::set_bar_color(ColorId color)
{
  bar_color = color;
}

//! Sets the color of the bar.
void
TimeBar::set_secondary_bar_color(ColorId color)
{
  secondary_bar_color = color;
}


//! Sets the color of the bar.
void
TimeBar::set_text_color(Gdk::Color color)
{
  bar_text_color = color;
}




//! Updates the screen.
void TimeBar::update()
{
  queue_draw();
}



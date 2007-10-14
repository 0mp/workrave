// FrameWindow.hh --- Gtk::Frame like widget
//
// Copyright (C) 2001, 2002, 2003, 2004, 2007 Raymond Penners <raymond@dotsphinx.com>
// All rights reserved.
//
// Time-stamp: <2007-10-08 20:25:29 robc>
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

#include <gtkmm/frame.h>

class Frame : public Gtk::Bin
{
public:
  enum Style
  {
    STYLE_SOLID,
    STYLE_BREAK_WINDOW
  };

  Frame();
  virtual ~Frame();

  void set_frame_width(guint width);
  void set_frame_style(Style style);
  void set_frame_color(const Gdk::Color &color);
  void set_frame_flashing(int delay);
  void set_frame_visible(bool visible);
  SigC::Signal1<void,bool> &signal_flash();

protected:
  bool on_timer();
  void on_size_request(Gtk::Requisition *requisition);
  void on_size_allocate(Gtk::Allocation &allocation);

  bool on_expose_event(GdkEventExpose* e);
  void on_realize();

private:
  //! Frame border width
  guint frame_width;

  //! Graphic context.
  Glib::RefPtr<Gdk::GC> gc;

  //! Color map
  Glib::RefPtr<Gdk::Colormap> color_map;

  //! Color of the frame.
  Gdk::Color frame_color;

  //! Black
  Gdk::Color color_black;

  //! Style of the frame.
  Style frame_style;

  //! Visible;
  bool frame_visible;

  //! Flash delay;
  int flash_delay;

  //! Flash timeout signal
  SigC::Connection flash_signal;

  //! Flash signal source
  SigC::Signal1<void,bool> flash_signal_src;
};


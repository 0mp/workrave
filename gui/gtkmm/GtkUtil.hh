// GtkUtil.hh --- Gtk utilities
//
// Copyright (C) 2003 Raymond Penners <raymond@dotsphinx.com>
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

#ifndef GTKUTIL_HH
#define GTKUTIL_HH

#include <gtkmm.h>
#include "GUIControl.hh"

class GtkUtil
{
public:
  static Gtk::Button *
  create_stock_button_without_text(const Gtk::StockID& stock_id);

  static Gtk::Widget *
  create_label_with_icon(const char *text, const char *icon);

  static Gtk::Widget *
  GtkUtil::create_label_with_tooltip(const char *text, const char *tooltip);

  static Gtk::Widget *
  create_label_for_break(GUIControl::BreakId id);

  static void
  table_attach_aligned(Gtk::Table &table, Gtk::Widget &child,
                       guint left_attach, guint top_attach, bool left);

  static void
  table_attach_left_aligned(Gtk::Table &table, Gtk::Widget &child,
                            guint left_attach, guint top_attach);

  static void
  table_attach_right_aligned(Gtk::Table &table, Gtk::Widget &child,
                             guint left_attach, guint top_attach);
};

#endif // GTKMMGUI_HH

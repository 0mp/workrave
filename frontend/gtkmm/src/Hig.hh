// Hig.hh --- Gnome HIG stuff
//
// Copyright (C) 2003, 2004 Raymond Penners <raymond@dotsphinx.com>
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

#ifndef HIG_HH
#define HIG_HH

#include <gtkmm/dialog.h>
#include <gtkmm/box.h>
#include <gtkmm/sizegroup.h>

class HigDialog : public Gtk::Dialog
{
public:
  HigDialog();
  HigDialog(const Glib::ustring& title, bool modal=false,
            bool use_separator=false);
  Gtk::VBox *get_vbox();

private:
  void set_hig_defaults();

  Gtk::VBox *vbox;
};

class HigCategoryPanel : public Gtk::VBox
{
public:
  HigCategoryPanel(Gtk::Widget &lab);
  HigCategoryPanel(const char *lab);
  HigCategoryPanel();
  Gtk::Label *add(const char *lab, Gtk::Widget &widget);
  void add(Gtk::Label &label, Gtk::Widget &widget);
  void add(Gtk::Widget &widget);

  void add_caption(Gtk::Widget &lab);
  void add_caption(const char *lab);

private:
  void init(Gtk::Widget &lab);

  Gtk::VBox *options_box;
  Glib::RefPtr<Gtk::SizeGroup> size_group;
};

class HigCategoriesPanel : public Gtk::VBox
{
public:
  HigCategoriesPanel();
  void add(Gtk::Widget &panel);
};

class HigUtil
{
public:
  static Glib::ustring create_alert_text(const char *caption,
                                         const char *body);
};

#endif // HIG_HH

// AppletPreferencePage.cc --- Preferences widgets for a timer
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
// $Id$
//

#include "preinclude.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "nls.h"
#include "debug.hh"

#include "AppletPreferencePage.hh"

#include "Configurator.hh"
#include "AppletWindow.hh"


//! Constructs the Applet Preference Notebook page.
AppletPreferencePage::AppletPreferencePage()
  : Gtk::HBox(false, 6)
{
  TRACE_ENTER("AppletPreferencePage::AppletPreferencePage");

  create_page();
  init_page_values();
  
  TRACE_EXIT();
}


//! Destructs the Applet Preference Notebook page.
AppletPreferencePage::~AppletPreferencePage()
{
  TRACE_ENTER("AppletPreferencePage::~AppletPreferencePage");
  TRACE_EXIT();
}


//! Initializes all widgets.
void
AppletPreferencePage::create_page()
{
  // Frame
  Gtk::Frame *frame = manage(new Gtk::Frame(_("Layout")));
  Gtk::VBox *box = manage(new Gtk::VBox());
  
  // Labels
  Gtk::Label *mp_label = manage(new Gtk::Label(_("Micro-pause")));
  Gtk::Label *rb_label = manage(new Gtk::Label(_("Restbreak")));
  Gtk::Label *dl_label = manage(new Gtk::Label(_("Daily limit")));
  Gtk::Label *visible_label = manage(new Gtk::Label(_("Break visible")));
  Gtk::Label *cycle_label = manage(new Gtk::Label(_("Cycle time")));
  Gtk::Label *slot_label = manage(new Gtk::Label(_("Break position")));
  Gtk::Label *first_label = manage(new Gtk::Label(_("Show only when first")));
  Gtk::Label *imminent_label = manage(new Gtk::Label(_("Show only when imminent in")));
  Gtk::Label *exclusive_label = manage(new Gtk::Label(_("Show exclusively")));
  Gtk::Label *default_label = manage(new Gtk::Label(_("Default break")));

  // Cycle time spin button.
  cycle_entry = manage(new Gtk::SpinButton());
  cycle_entry->set_range(1, 999);
  cycle_entry->set_increments(1, 10);
  cycle_entry->set_numeric(true);
  cycle_entry->set_width_chars(3);
  cycle_entry->signal_changed().connect(SigC::slot(*this, &AppletPreferencePage::on_cycle_time_changed));

  // Enabled/Disabled checkbox
  enabled_cb = manage(new  Gtk::CheckButton("Applet enabled"));
  enabled_cb->signal_toggled().connect(SigC::slot(*this, &AppletPreferencePage::on_enabled_toggled));

  // Table
  Gtk::Table *table = manage(new Gtk::Table(8, 4, false));
  table->set_row_spacings(2);
  table->set_col_spacings(6);
  table->set_border_width(6);

  // Tooltips;
  Gtk::Tooltips *tips = manage(new Gtk::Tooltips());
  tips->enable();
  tips->set_tip(*enabled_cb, _("Applet enabled or not."));
  tips->set_tip(*cycle_entry, _("When multiple breaks are shown on a certain position, "
                                "this value indicates the time each break is shown."));

  // Put the breaks in table.
  for (int i = 0; i < GUIControl::BREAK_ID_SIZEOF; i++)
    {
      slot_entry[i] = manage(new Gtk::SpinButton());
      slot_entry[i]->set_range(0, 3);
      slot_entry[i]->set_increments(1, 1);
      slot_entry[i]->set_numeric(true);
      slot_entry[i]->set_width_chars(2);

      time_entry[i] = manage(new Gtk::SpinButton());
      time_entry[i]->set_range(0, 3600);
      time_entry[i]->set_increments(1, 10);
      time_entry[i]->set_numeric(true);
      time_entry[i]->set_width_chars(4);
      
      visible_cb[i] = manage(new Gtk::CheckButton());
      first_cb[i] = manage(new Gtk::CheckButton());
      imminent_cb[i] = manage(new Gtk::CheckButton());
      default_cb[i] = manage(new Gtk::CheckButton());
      exclusive_cb[i] = manage(new Gtk::CheckButton());

      Gtk::HBox *hbox = manage(new Gtk::HBox());
      hbox->pack_start(*imminent_cb[i], false, false, 0);
      hbox->pack_start(*time_entry[i], false, false, 0);

      int y = 2;
      table->attach(*visible_cb[i], i + 1, i + 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
      y++;
      table->attach(*slot_entry[i], i + 1, i + 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
      y++;
      table->attach(*default_cb[i], i + 1, i + 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
      y++;
      table->attach(*exclusive_cb[i], i + 1, i + 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
      y++;
      table->attach(*first_cb[i], i + 1, i + 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
      y++;
      table->attach(*hbox, i + 1, i + 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
      y++;

      visible_cb[i]->signal_toggled().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_visible_toggled), i));
      first_cb[i]->signal_toggled().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_first_toggled), i));
      exclusive_cb[i]->signal_toggled().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_exclusive_toggled), i));
      default_cb[i]->signal_toggled().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_default_toggled), i));
      imminent_cb[i]->signal_toggled().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_imminent_toggled), i));
      time_entry[i]->signal_changed().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_time_changed), i));
      slot_entry[i]->signal_changed().connect(bind(SigC::slot(*this, &AppletPreferencePage::on_slot_changed), i));

      tips->set_tip(*visible_cb[i], _("If set, this break will be shown in the applet."));
      
      tips->set_tip(*slot_entry[i], _("Position on which the break timer is shown. Three positions "
                                      "are available. In case multiple breaks have the same position, "
                                      "they will be shown alternatingly."));
      
      tips->set_tip(*default_cb[i], _("If set, this break will be shown if no other breaks "
                                      "are shown on the selected position"));

      tips->set_tip(*exclusive_cb[i], _("If set, the break timer will always be the only "
                                        "break that will be shown on that position. It will not "
                                        "be shown alternatingly with another break."));

      tips->set_tip(*first_cb[i], _("If set, the break timer will only be shown if it is the "
                                    "first break that will reach its limit."));

      tips->set_tip(*imminent_cb[i], _("If set, the break timer will only be shown if it will "
                                       "reach its limit within the specified time."));
      
      tips->set_tip(*time_entry[i], _("If set, the break timer will only be shown if it will "
                                      "reach its limit within the specified time."));
    }

  // Add labels to table.
  int y = 0;
  
  table->attach(*cycle_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  table->attach(*cycle_entry, 1, 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  
  table->attach(*mp_label, 1, 2, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  table->attach(*rb_label, 2, 3, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  table->attach(*dl_label, 3, 4, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  
  table->attach(*visible_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  table->attach(*slot_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  table->attach(*default_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  table->attach(*exclusive_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  table->attach(*first_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  table->attach(*imminent_label, 0, 1, y, y + 1, Gtk::SHRINK, Gtk::SHRINK);
  y++;

  // And put it all together.
  box->pack_start(*enabled_cb, false, false, 0);
  box->pack_start(*frame, true, true, 0);
  frame->add(*table);
  pack_end(*box, true, true, 0);
}


//! Sets the specified flag for the specified break.
void
AppletPreferencePage::set_flag(int break_id, int flag, bool on)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  Configurator *c = GUIControl::get_instance()->get_configurator();
  GUIControl::TimerData &data = GUIControl::get_instance()->timers[break_id];
      
  int value = 0;
  c->get_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
               + AppletWindow::CFG_KEY_APPLET_FLAGS,
               &value);

  if (on)
    {
      value |= flag;
    }
  else
    {
      value &= ~flag;
    }

  c->set_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
               + AppletWindow::CFG_KEY_APPLET_FLAGS,
               value);
}


//! The 'visible' Checkbox of the specified break is toggled.
void
AppletPreferencePage::on_visible_toggled(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  bool visible = visible_cb[break_id]->get_active();
  int value = -1;
  Configurator *c = GUIControl::get_instance()->get_configurator();

  if (visible)
    {
      slot_entry[break_id]->set_sensitive(true);
      value = (int) slot_entry[break_id]->get_value();
    }
  else
    {
      slot_entry[break_id]->set_sensitive(false);
    }

  GUIControl::TimerData &data = GUIControl::get_instance()->timers[break_id];
      
  c->set_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
               + AppletWindow::CFG_KEY_APPLET_POSITION,
               value);
}


//! The 'first' Checkbox of the specified break is toggled.
void
AppletPreferencePage::on_first_toggled(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  bool first = first_cb[break_id]->get_active();
  set_flag(break_id, AppletWindow::BREAK_WHEN_FIRST, first);
}


//! The 'imminent' Checkbox of the specified break is toggled.
void
AppletPreferencePage::on_imminent_toggled(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  bool imminent = imminent_cb[break_id]->get_active();
  set_flag(break_id, AppletWindow::BREAK_WHEN_IMMINENT, imminent);

  time_entry[break_id]->set_sensitive(imminent);
}


//! The 'default' Checkbox of the specified break is toggled.
void
AppletPreferencePage::on_default_toggled(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  bool d = default_cb[break_id]->get_active();
  set_flag(break_id, AppletWindow::BREAK_DEFAULT, d);
}


//! The 'exclusive' Checkbox of the specified break is toggled.
void
AppletPreferencePage::on_exclusive_toggled(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  bool exclusive = exclusive_cb[break_id]->get_active();
  set_flag(break_id, AppletWindow::BREAK_EXCLUSIVE, exclusive);
}


//! The 'imminent time' of the specified break is changed.
void
AppletPreferencePage::on_time_changed(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  int value = (int) time_entry[break_id]->get_value();
  
  Configurator *c = GUIControl::get_instance()->get_configurator();
  GUIControl::TimerData &data = GUIControl::get_instance()->timers[break_id];
      
  c->set_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
               + AppletWindow::CFG_KEY_APPLET_IMMINENT,
               value);
  
}


//! The 'position' of the specified break is changed.
void
AppletPreferencePage::on_slot_changed(int break_id)
{
  assert(break_id >= 0 && break_id < GUIControl::BREAK_ID_SIZEOF);

  int value = (int) slot_entry[break_id]->get_value();
  
  Configurator *c = GUIControl::get_instance()->get_configurator();
  GUIControl::TimerData &data = GUIControl::get_instance()->timers[break_id];
      
  c->set_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
               + AppletWindow::CFG_KEY_APPLET_POSITION,
               value);
}


//! The applet on/off checkbox has been toggled.
void
AppletPreferencePage::on_enabled_toggled()
{
  bool on = enabled_cb->get_active();
  
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(AppletWindow::CFG_KEY_APPLET_ENABLED, on);
}


//! The applet cycle time has been changed.
void
AppletPreferencePage::on_cycle_time_changed()
{
  int value = (int) cycle_entry->get_value();
  
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(AppletWindow::CFG_KEY_APPLET_CYCLE_TIME, value);
}


//! Retrieves the applet configuration and sets the widgets.
void
AppletPreferencePage::init_page_values()
{
  Configurator *c = GUIControl::get_instance()->get_configurator();

  // Enabled 
  bool enabled = false;
  if (!c->get_value(AppletWindow::CFG_KEY_APPLET_ENABLED, &enabled))
    {
      enabled = false;
    }
  enabled_cb->set_active(enabled);
  
  // Cycle time. 
  int value = 10;
  if (!c->get_value(AppletWindow::CFG_KEY_APPLET_CYCLE_TIME, &value))
    {
      value = 10;
    }
  cycle_entry->set_value(value);

  // Break specific configuration
  for (int i = 0; i < GUIControl::BREAK_ID_SIZEOF; i++)
    {
      GUIControl::TimerData &data = GUIControl::get_instance()->timers[i];

      // Break position
      int value = 0;
      if (!c->get_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
                        + AppletWindow::CFG_KEY_APPLET_POSITION, &value))
        {
          value = i;
        }

      if (value >= 0)
        {
          slot_entry[i]->set_sensitive(true);
          slot_entry[i]->set_value(value);
          visible_cb[i]->set_active(true);
        }
      else
        {
          slot_entry[i]->set_sensitive(false);
          visible_cb[i]->set_active(false);
        }

      // Flags
      if (!c->get_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
                        + AppletWindow::CFG_KEY_APPLET_FLAGS, &value))
        {
          value = AppletWindow::BREAK_EXCLUSIVE;
        }

      imminent_cb[i]->set_active(value & AppletWindow::BREAK_WHEN_IMMINENT);
      first_cb[i]->set_active(value & AppletWindow::BREAK_WHEN_FIRST);
      exclusive_cb[i]->set_active(value & AppletWindow::BREAK_EXCLUSIVE);
      default_cb[i]->set_active(value & AppletWindow::BREAK_DEFAULT);
      time_entry[i]->set_sensitive(value & AppletWindow::BREAK_WHEN_IMMINENT);

      // Imminent time threshold.
      if (!c->get_value(AppletWindow::CFG_KEY_APPLET + "/" + data.break_name
                        + AppletWindow::CFG_KEY_APPLET_IMMINENT, &value))
        {
          value = 30;
        }

      time_entry[i]->set_value(value);
    }
}

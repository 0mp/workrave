// TimerBoxPreferencesPanel.cc --- base class for the break windows
//
// Copyright (C) 2001 - 2013 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include "TimerBoxPreferencesPanel.hh"

#include <QtGui>
#include <QStyle>

#include "debug.hh"
#include "nls.h"

// #include "IBreak.hh"
// #include "config/IConfigurator.hh"
// #include "CoreFactory.hh"
// #include "ICore.hh"
#include "CoreConfig.hh"
// #include "GtkUtil.hh"
// #include "Hig.hh"
#include "GUIConfig.hh"
#include "TimerBoxControl.hh"
// #include "Locale.hh"
// #include "System.hh"
// #include "Util.hh"
// #include "ICore.hh"
#include "UiUtil.hh"
#include "CoreFactory.hh"

using namespace workrave;
using namespace workrave::config;

TimerBoxPreferencesPanel::TimerBoxPreferencesPanel(std::string name)
  : name(name)
{
  TRACE_ENTER("TimerBoxPreferencesPanel::TimerBoxPreferencesPanel");

  connector = new DataConnector();

  layout = new QVBoxLayout;
  setLayout(layout);

  create_page();
  init_page_values();
  enable_buttons();
  init_page_callbacks();
  
  layout->addStretch();

  IConfigurator::Ptr config = CoreFactory::get_configurator();
  config->add_listener(TimerBoxControl::CFG_KEY_TIMERBOX + name, this);

  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      ICore::Ptr core = CoreFactory::get_core();
      config->add_listener(CoreConfig::CFG_KEY_BREAK_ENABLED % BreakId(i), this);
    }

  
  TRACE_EXIT();
}

TimerBoxPreferencesPanel::~TimerBoxPreferencesPanel()
{
  TRACE_ENTER("TimerBoxPreferencesPanel::~TimerBoxPreferencePanel");

  IConfigurator::Ptr config = CoreFactory::get_configurator();
  config->remove_listener(this);

  TRACE_EXIT();
}

//! Initializes all widgets.
void
TimerBoxPreferencesPanel::create_page()
{
  // Placement
  place_button = new QComboBox();
  place_button->addItem(_("Place timers next to each other"));
  place_button->addItem(_("Place micro-break and rest break in one spot"));
  place_button->addItem(_("Place rest break and daily limit in one spot"));
  place_button->addItem(_("Place all timers in one spot"));

  // Cycle time spin button.
  cycle_entry = new QSpinBox;
  cycle_entry->setMinimum(1);
  cycle_entry->setMaximum(999);

  void (QSpinBox:: *signal)(int) = &QSpinBox::valueChanged;
  connect(cycle_entry, signal, this, &TimerBoxPreferencesPanel::on_cycle_time_changed);
  
  // Timer display
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      QComboBox *display_button  = new QComboBox;
      timer_display_button[i] = display_button;

      display_button->addItem(_("Hide"));
      display_button->addItem(_("Show"));
      display_button->addItem(_("Show only when this timer is first due"));
    }

  // Enabled/Disabled checkbox
  enabled_cb = new QCheckBox();
  ontop_cb = NULL;
  
  if (name == "main_window")
    {
      enabled_cb->setText(_("Show status window"));

      // Always-on-top
      ontop_cb = new QCheckBox;
      ontop_cb->setText(_("The status window stays always on top of other windows"));
      ontop_cb->setCheckState(GUIConfig::get_always_on_top() ? Qt::Checked : Qt::Unchecked);

      connect(ontop_cb, &QCheckBox::stateChanged, this, &TimerBoxPreferencesPanel::on_always_on_top_toggled);
    }
  else if (name == "applet")
    {
      enabled_cb->setText(_("Applet enabled"));
    }

  layout->addWidget(enabled_cb);
  
  QGroupBox *display_box = new QGroupBox(_("Display"));
  QVBoxLayout *display_layout = new QVBoxLayout;
  display_box->setLayout(display_layout);

  layout->addWidget(display_box);
  
  display_layout->addWidget(enabled_cb);

  if (ontop_cb != NULL)
    {
      display_layout->addWidget(ontop_cb);
    }

  UiUtil::add_widget(display_layout, _("Placement:"), place_button);
  UiUtil::add_widget(display_layout, _("Cycle time:"), cycle_entry);

  QGroupBox *timers_box = new QGroupBox(_("Timers"));
  QVBoxLayout *timers_layout = new QVBoxLayout;
  timers_box->setLayout(timers_layout);
  layout->addWidget(timers_box);
  
  // Layout
  UiUtil::add_widget(timers_layout, _("Micro-break:"), timer_display_button[0]);
  UiUtil::add_widget(timers_layout, _("Rest break:"), timer_display_button[1]);
  UiUtil::add_widget(timers_layout, _("Daily limit:"), timer_display_button[2]);
}


//! Retrieves the applet configuration and sets the widgets.
void
TimerBoxPreferencesPanel::init_page_values()
{
  int mp_slot = TimerBoxControl::get_timer_slot(name, BREAK_ID_MICRO_BREAK);
  int rb_slot = TimerBoxControl::get_timer_slot(name, BREAK_ID_REST_BREAK);
  int dl_slot = TimerBoxControl::get_timer_slot(name, BREAK_ID_DAILY_LIMIT);
  int place;
  if (mp_slot < rb_slot && rb_slot < dl_slot)
    {
      place = 0;
    }
  else if (mp_slot == rb_slot && rb_slot == dl_slot)
    {
      place = 3;
    }
  else if (mp_slot == rb_slot)
    {
      place = 1;
    }
  else
    {
      place = 2;
    }
  place_button->setCurrentIndex(place);


  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      int flags = TimerBoxControl::get_timer_flags(name, (BreakId) i);
      int showhide;
      if (flags & TimerBoxControl::BREAK_HIDE)
        {
          showhide = 0;
        }
      else if (flags & TimerBoxControl::BREAK_WHEN_FIRST)
        {
          showhide = 2;
        }
      else
        {
          showhide = 1;
        }
      timer_display_button[i]->setCurrentIndex(showhide);
    }
  cycle_entry->setValue(TimerBoxControl::get_cycle_time(name));

  enabled_cb->setCheckState(TimerBoxControl::is_enabled(name) ? Qt::Checked : Qt::Unchecked); 
  enable_buttons();
}


void
TimerBoxPreferencesPanel::init_page_callbacks()
{
  void (QComboBox:: *signal)(int) = &QComboBox::currentIndexChanged;
  QObject::connect(place_button, signal, this, &TimerBoxPreferencesPanel::on_enabled_toggled);

  connect(enabled_cb, &QCheckBox::stateChanged, this, &TimerBoxPreferencesPanel::on_enabled_toggled);
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      // Todo BIND i.
      QObject::connect(timer_display_button[i], signal, this, &TimerBoxPreferencesPanel::on_display_changed);
    }
}

//! The applet on/off checkbox has been toggled.
void
TimerBoxPreferencesPanel::on_enabled_toggled()
{
  bool on = enabled_cb->checkState() == Qt::Checked;

  TimerBoxControl::set_enabled(name, on);

  enable_buttons();
}


//! The placement is changed.
void
TimerBoxPreferencesPanel::on_place_changed()
{
  int slot[BREAK_ID_SIZEOF];
  int idx = place_button->currentIndex();
  switch (idx)
    {
    case 0:
      slot[BREAK_ID_MICRO_BREAK] = 0;
      slot[BREAK_ID_REST_BREAK] = 1;
      slot[BREAK_ID_DAILY_LIMIT] = 2;
      break;
    case 1:
      slot[BREAK_ID_MICRO_BREAK] = 0;
      slot[BREAK_ID_REST_BREAK] = 0;
      slot[BREAK_ID_DAILY_LIMIT] = 1;
      break;
    case 2:
      slot[BREAK_ID_MICRO_BREAK] = 0;
      slot[BREAK_ID_REST_BREAK] = 1;
      slot[BREAK_ID_DAILY_LIMIT] = 1;
      break;
    case 3:
      slot[BREAK_ID_MICRO_BREAK] = 0;
      slot[BREAK_ID_REST_BREAK] = 0;
      slot[BREAK_ID_DAILY_LIMIT] = 0;
      break;
    default:
      slot[BREAK_ID_MICRO_BREAK] = -1;
      slot[BREAK_ID_REST_BREAK] = -1;
      slot[BREAK_ID_DAILY_LIMIT] = -1;
    }

  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      TimerBoxControl::set_timer_slot(name, (BreakId) i, slot[i]);
    }
}


//! The display of the specified break is changed.
void
TimerBoxPreferencesPanel::on_display_changed(int break_id)
{
  int sel = timer_display_button[break_id]->currentIndex();
  int flags = 0;
  switch (sel)
    {
    case 0:
      flags |= TimerBoxControl::BREAK_HIDE;
      break;
    case 1:
      flags = 0;
      break;
    default:
      flags = TimerBoxControl::BREAK_WHEN_FIRST;
      break;
    }
  TimerBoxControl::set_timer_flags(name, (BreakId) break_id, flags);

  enable_buttons();
}


//! Enable widgets
void
TimerBoxPreferencesPanel::enable_buttons(void)
{
  int count = 0;
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      if (timer_display_button[i]->currentIndex() == 0)
        {
          count++;
        }
    }

  if (name == "applet")
    {
      bool on = enabled_cb->checkState() == Qt::Checked;

      place_button->setEnabled(on && count != 3);
      for (int i = 0; i < BREAK_ID_SIZEOF; i++)
        {
          ICore::Ptr core = CoreFactory::get_core();
          IBreak::Ptr b = core->get_break(BreakId(i));

          bool timer_on = b->is_enabled();
          timer_display_button[i]->setEnabled(on && timer_on);
        }
      cycle_entry->setEnabled(on && count != 3);

    }
  else if (name == "main_window")
    {
      for (int i = 0; i < BREAK_ID_SIZEOF; i++)
        {
          ICore::Ptr core = CoreFactory::get_core();
          IBreak::Ptr b = core->get_break(BreakId(i));
          timer_display_button[i]->setEnabled(b->is_enabled());
        }
      if (count == 3)
        {
          if (TimerBoxControl::is_enabled(name))
            {
              TimerBoxControl::set_enabled(name, false);
            }
          enabled_cb->setCheckState(Qt::Checked);
        }
      enabled_cb->setEnabled(count != 3);
      place_button->setEnabled(count != 3);
      cycle_entry->setEnabled(count != 3);
      ontop_cb->setEnabled(count != 3);
    }
}


//! The applet cycle time has been changed.
void
TimerBoxPreferencesPanel::on_cycle_time_changed()
{
  int value = (int) cycle_entry->value();
  TimerBoxControl::set_cycle_time(name, value);
}


void
TimerBoxPreferencesPanel::on_always_on_top_toggled()
{
  GUIConfig::set_always_on_top(ontop_cb->checkState() == Qt::Checked);
}


void
TimerBoxPreferencesPanel::config_changed_notify(const std::string &key)
{
  TRACE_ENTER("TimerBoxPreferencesPanel::config_changed_notify");
  (void)key;
  enable_buttons();
  TRACE_EXIT();
}

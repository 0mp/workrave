// PreferencesDialog.cc --- Preferences dialog
//
// Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008 Raymond Penners <raymond@dotsphinx.com>
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
// $Id$

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "preinclude.h"

#include "nls.h"
#include "debug.hh"

#include <unistd.h>
#include <assert.h>

#include <gtkmm/notebook.h>
#include <gtkmm/stock.h>
#include <gtkmm/menu.h>
#include <gtkmm/optionmenu.h>
#include <gtkmm/cellrenderer.h>

#include "StringUtil.hh"
#include "Locale.hh"

#include "GtkUtil.hh"
#include "Hig.hh"
#include "MainWindow.hh"
#include "PreferencesDialog.hh"
#include "SoundPlayer.hh"
#include "TimeEntry.hh"
#include "TimerBoxPreferencePage.hh"
#include "TimerPreferencesPanel.hh"
#include "Util.hh"
#include "GUI.hh"
#include "GUIConfig.hh"

#include "CoreFactory.hh"
#include "IConfigurator.hh"

#ifdef HAVE_DISTRIBUTION
#include "NetworkPreferencePage.hh"
#endif

// #include "PluginsPreferencePage.hh"


using namespace std;

PreferencesDialog::PreferencesDialog()
  : HigDialog(_("Preferences"), false, false)
{
  TRACE_ENTER("PreferencesDialog::PreferencesDialog");

  // Pages
  Gtk::Widget *timer_page = manage(create_timer_page());
  Gtk::Notebook *gui_page = manage(new Gtk::Notebook());

#if !defined(PLATFORM_OS_OSX)
  Gtk::Widget *gui_general_page = manage(create_gui_page());
  gui_page->append_page(*gui_general_page, _("General"));
#endif
  
  Gtk::Widget *gui_mainwindow_page = manage(create_mainwindow_page());
  gui_page->append_page(*gui_mainwindow_page, _("Status Window"));

#if !defined(PLATFORM_OS_OSX)
  Gtk::Widget *gui_applet_page = manage(create_applet_page());
  gui_page->append_page(*gui_applet_page, _("Applet"));
#endif
  
#ifdef HAVE_DISTRIBUTION
  Gtk::Widget *network_page = manage(create_network_page());
#endif

  // Notebook
  add_page(_("Timers"), "time.png", *timer_page);
  add_page(_("User interface"), "display.png", *gui_page);
#ifdef HAVE_DISTRIBUTION
  add_page(_("Network"), "network.png", *network_page);
#endif

#if 0  
  Gtk::Widget *plugins_page = manage( new PluginsPreferencePage() );
  add_page( _("Plugins"), "workrave-icon-huge.png", *plugins_page );
#endif
  
  // Dialog
  get_vbox()->pack_start(notebook, true, true, 0);
  add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);

  ICore *core = CoreFactory::get_core();
  mode = core->get_operation_mode();

#ifdef PLATFORM_OS_UNIX
  GtkUtil::set_wmclass(*this, "Preferences");
#endif

  show_all();

  TRACE_EXIT();
}


//! Destructor.
PreferencesDialog::~PreferencesDialog()
{
  TRACE_ENTER("PreferencesDialog::~PreferencesDialog");

#if defined(PLATFORM_OS_WIN32)
  const Gtk::TreeModel::iterator& iter = languages_combo.get_active();
  const Gtk::TreeModel::Row row = *iter;
  const Glib::ustring code = row[languages_columns.code];

  GUIConfig::set_locale(code);
#endif
  
  ICore *core = CoreFactory::get_core();
  core->set_operation_mode(mode);

  TRACE_EXIT();
}



Gtk::Widget *
PreferencesDialog::create_gui_page()
{
  // Sound types
  sound_button  = manage(new Gtk::OptionMenu());
  Gtk::Menu *sound_menu = manage(new Gtk::Menu());
  Gtk::Menu::MenuList &sound_list = sound_menu->items();
  sound_button->set_menu(*sound_menu);
  sound_list.push_back(Gtk::Menu_Helpers::MenuElem(_("No sounds")));
  sound_list.push_back(Gtk::Menu_Helpers::MenuElem
                       (_("Play sounds using sound card")));
  sound_list.push_back(Gtk::Menu_Helpers::MenuElem
                       (_("Play sounds using built-in speaker")));
  int idx;
  if (! SoundPlayer::is_enabled())
    idx = 0;
  else
    {
      if (SoundPlayer::DEVICE_SPEAKER == SoundPlayer::get_device())
        idx = 2;
      else
        idx = 1;
    }
  sound_button->set_history(idx);
  sound_button->signal_changed().connect(sigc::mem_fun(*this, &PreferencesDialog::on_sound_changed));

  // Block types
  block_button  = manage(new Gtk::OptionMenu());
  Gtk::Menu *block_menu = manage(new Gtk::Menu());
  Gtk::Menu::MenuList &block_list = block_menu->items();
  block_button->set_menu(*block_menu);
  block_list.push_back(Gtk::Menu_Helpers::MenuElem(_("No blocking")));
  block_list.push_back(Gtk::Menu_Helpers::MenuElem
                       (_("Block input")));
  block_list.push_back(Gtk::Menu_Helpers::MenuElem
                       (_("Block input and screen")));

  int block_idx;
  switch (GUIConfig::get_block_mode())
    {
    case GUIConfig::BLOCK_MODE_NONE:
      block_idx = 0;
      break;
    case GUIConfig::BLOCK_MODE_INPUT:
      block_idx = 1;
      break;
    default:
      block_idx = 2;
    }
  block_button->set_history(block_idx);
  block_button->signal_changed()
    .connect(sigc::mem_fun(*this, &PreferencesDialog::on_block_changed));

  // Options
  HigCategoryPanel *panel = manage(new HigCategoryPanel(_("Options")));

  panel->add(_("Sound:"), *sound_button);
  panel->add(_("Block mode:"), *block_button);
  
#if defined(PLATFORM_OS_WIN32)
  string current_locale = GUIConfig::get_locale();

  languages_model = Gtk::ListStore::create(languages_columns);
  languages_combo.set_model(languages_model);
  
  std::vector<std::string> all_linguas;
  StringUtil::split(string(ALL_LINGUAS), ' ', all_linguas);
  all_linguas.push_back("en");
  
  Locale::LanguageMap languages_current_locale;
  Locale::LanguageMap languages_native_locale;
  
  Locale::get_all_languages_in_current_locale(languages_current_locale);
  Locale::get_all_languages_in_native_locale(languages_native_locale);

  Gtk::TreeModel::iterator iter = languages_model->append();
  Gtk::TreeModel::Row row = *iter;
  row[languages_columns.current] = _("System default");
  row[languages_columns.native] = "";
  row[languages_columns.code] = "";
  row[languages_columns.enabled] = true;

  Gtk::TreeModel::iterator selected = iter;  

  for (vector<std::string>::iterator i = all_linguas.begin(); i != all_linguas.end(); i++)
    {
      string code = *i;

      iter = languages_model->append();
      row = *iter;
      row[languages_columns.code] = code;
      row[languages_columns.enabled] = true;

      if (current_locale == code)
        {
          selected = iter;
        }
      
      string txt = languages_current_locale[code].language_name;
      if (languages_current_locale[code].country_name != "")
        {
          txt += " (" + languages_current_locale[code].country_name + ")";
        }
      row[languages_columns.current] = txt;

      if (languages_current_locale[code].language_name !=
          languages_native_locale[code].language_name)
        {
          txt = languages_native_locale[code].language_name;
          if (languages_native_locale[code].country_name != "")
            {
              txt += " (" + languages_native_locale[code].country_name + ")";
            }

          Glib::RefPtr<Pango::Layout> pl = create_pango_layout(txt);
          if (pl->get_unknown_glyphs_count() > 0)
            {
              txt = _("(font not available)");
              row[languages_columns.enabled] = false;
            }
          
          row[languages_columns.native] = txt;
        }
    }

  languages_model->set_sort_column(languages_columns.current, Gtk::SORT_ASCENDING);
 	languages_model->set_sort_func (languages_columns.current,
                                  sigc::mem_fun(*this,
                                                &PreferencesDialog::on_cell_data_compare));
    
  languages_combo.pack_start(current_cellrenderer, true); 
  languages_combo.pack_start(native_cellrenderer, false);
  
  languages_combo.set_cell_data_func(native_cellrenderer,
                                     sigc::mem_fun(*this,
                                                   &PreferencesDialog::on_native_cell_data));
  languages_combo.set_cell_data_func(current_cellrenderer,
                                     sigc::mem_fun(*this,
                                                   &PreferencesDialog::on_current_cell_data));

  languages_combo.set_active(selected);
  
  panel->add(_("Language:"), languages_combo);
#endif
  
  panel->set_border_width(12);
  return panel;
}


void
PreferencesDialog::on_current_cell_data(const Gtk::TreeModel::const_iterator& iter)
{
  if (iter)
  {
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring name = row[languages_columns.current];
    bool enabled = row[languages_columns.enabled];

    current_cellrenderer.set_property("text", name);
    current_cellrenderer.set_property("sensitive", enabled);
  }
}

void
PreferencesDialog::on_native_cell_data(const Gtk::TreeModel::const_iterator& iter)
{
  if (iter)
  {
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring name = row[languages_columns.native];
    bool enabled = row[languages_columns.enabled];

    native_cellrenderer.set_property("text", name);
    native_cellrenderer.set_property("sensitive", enabled);
  }
}

int
PreferencesDialog::on_cell_data_compare(const Gtk::TreeModel::iterator& iter1,
                                        const Gtk::TreeModel::iterator& iter2)
{
  Gtk::TreeModel::Row row1 = *iter1;
  Gtk::TreeModel::Row row2 = *iter2;
  Glib::ustring name1 = row1[languages_columns.current];
  Glib::ustring name2 = row2[languages_columns.current];
  Glib::ustring code1 = row1[languages_columns.code];
  Glib::ustring code2 = row2[languages_columns.code];

  if (code1 == "")
    {
      return -1;
    }
  else if (code2 == "")
    {
      return 1;
    }
  else
    {
      return g_utf8_collate(name1.c_str(), name2.c_str());
    }
}


Gtk::Widget *
PreferencesDialog::create_timer_page()
{
  // Timers page
  Gtk::Notebook *tnotebook = manage(new Gtk::Notebook());
  tnotebook->set_tab_pos (Gtk::POS_TOP);
  Glib::RefPtr<Gtk::SizeGroup> hsize_group
    = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
  Glib::RefPtr<Gtk::SizeGroup> vsize_group
    = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_VERTICAL);
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      // Label
      Gtk::Widget *box = manage(GtkUtil::create_label_for_break
                                ((BreakId) i));
      TimerPreferencesPanel *tp = manage(new TimerPreferencesPanel(BreakId(i), hsize_group, vsize_group));
      box->show_all();
      tnotebook->pages().push_back(Gtk::Notebook_Helpers::TabElem(*tp, *box));
    }
  return tnotebook;
}

Gtk::Widget *
PreferencesDialog::create_mainwindow_page()
{
  // Timers page
  return new TimerBoxPreferencePage("main_window");
}


Gtk::Widget *
PreferencesDialog::create_applet_page()
{
  // Timers page
  return new TimerBoxPreferencePage("applet");
}


#ifdef HAVE_DISTRIBUTION
Gtk::Widget *
PreferencesDialog::create_network_page()
{
  return new NetworkPreferencePage();
}
#endif

void
PreferencesDialog::add_page(const char *label, const char *image,
                            Gtk::Widget &widget)
{
  string icon = Util::complete_directory(image, Util::SEARCH_PATH_IMAGES);
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(icon);
  notebook.add_page(label, pixbuf, widget);
}

void
PreferencesDialog::on_sound_changed()
{
  int idx = sound_button->get_history();
  SoundPlayer::set_enabled(idx > 0);
  if (idx > 0)
    {
      SoundPlayer::Device dev = idx == 1
        ? SoundPlayer::DEVICE_SOUNDCARD
        : SoundPlayer::DEVICE_SPEAKER;
      SoundPlayer::set_device(dev);
    }
}

void
PreferencesDialog::on_block_changed()
{
  int idx = block_button->get_history();
  GUIConfig::BlockMode m;
  switch (idx)
    {
    case 0:
      m = GUIConfig::BLOCK_MODE_NONE;
      break;
    case 1:
      m = GUIConfig::BLOCK_MODE_INPUT;
      break;
    default:
      m = GUIConfig::BLOCK_MODE_ALL;
    }
  GUIConfig::set_block_mode(m);
}


void
PreferencesDialog::on_language_changed()
{
}



int
PreferencesDialog::run()
{
  show_all();
  return 0;
}


bool
PreferencesDialog::on_focus_in_event(GdkEventFocus *event)
{
  TRACE_ENTER("PreferencesDialog::focus_in");

  GUIConfig::BlockMode block_mode = GUIConfig::get_block_mode();
  if (block_mode != GUIConfig::BLOCK_MODE_NONE)
    {
      ICore *core = CoreFactory::get_core();

      mode = core->get_operation_mode();
      if (mode == OPERATION_MODE_NORMAL)
        {
          mode = core->set_operation_mode(OPERATION_MODE_QUIET);
        }
    }
  TRACE_EXIT();
  return HigDialog::on_focus_in_event(event);
}


bool
PreferencesDialog::on_focus_out_event(GdkEventFocus *event)
{
  TRACE_ENTER("PreferencesDialog::focus_out");
  ICore *core = CoreFactory::get_core();

  core->set_operation_mode(mode);
  TRACE_EXIT();
  return HigDialog::on_focus_out_event(event);
}

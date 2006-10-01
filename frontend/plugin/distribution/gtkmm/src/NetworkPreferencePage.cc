// NetworkPreferencePage.cc --- Preferences widgets for a timer
//
// Copyright (C) 2002, 2003, 2004, 2006 Rob Caelers & Raymond Penners
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "preinclude.h"

#include "nls.h"
#include "debug.hh"

#include <sstream>
#include <unistd.h>

#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/notebook.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treeviewcolumn.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stock.h>

#include "NetworkPreferencePage.hh"
#include "Hig.hh"
#include "GtkUtil.hh"

#include "ICore.hh"
#include "CoreFactory.hh"
#include "IConfigurator.hh"
#include "IDistributionManager.hh"

NetworkPreferencePage::NetworkPreferencePage()
  : Gtk::VBox(false, 6)
{
  TRACE_ENTER("NetworkPreferencePage::NetworkPreferencePage");

  Gtk::Notebook *tnotebook = manage(new Gtk::Notebook());
  tnotebook->set_tab_pos(Gtk::POS_TOP);  

  ICore *core = CoreFactory::get_core();
  dist_manager = core->get_distribution_manager();
  assert(dist_manager != NULL);
  
  create_general_page(tnotebook);
  create_peers_page(tnotebook);
  create_advanced_page(tnotebook);

  init_page_values();

  pack_start(*tnotebook, true, true, 0);

  tnotebook->show_all();
  tnotebook->set_current_page(0);

  TRACE_EXIT();
}


NetworkPreferencePage::~NetworkPreferencePage()
{
  TRACE_ENTER("NetworkPreferencePage::~NetworkPreferencePage");
  TRACE_EXIT();
}


void
NetworkPreferencePage::create_general_page(Gtk::Notebook *tnotebook)
{
  // Main switch
  enabled_cb = manage(new Gtk::CheckButton());
  Gtk::Label *ena_lab
    = manage(GtkUtil::create_label(_("Enable networking"), true));
  enabled_cb->add(*ena_lab);

  // Identity
  HigCategoryPanel *id_frame = manage(new HigCategoryPanel(*enabled_cb));
  username_entry = manage(new Gtk::Entry());
  password_entry = manage(new Gtk::Entry());
  id_frame->add(_("Username:"), *username_entry);
  id_frame->add(_("Password:"), *password_entry);
  password_entry->set_visibility(false);
  password_entry->set_invisible_char('*');
  
  id_frame->set_border_width(12);
  tnotebook->append_page(*id_frame, _("General"));

  enabled_cb->signal_toggled().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_enabled_toggled));
  username_entry->signal_changed().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_username_changed));
  password_entry->signal_changed().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_password_changed));
}


void
NetworkPreferencePage::create_advanced_page(Gtk::Notebook *tnotebook)
{
  HigCategoryPanel *advanced_frame
    = manage(new HigCategoryPanel(_("Server settings")));
  advanced_frame->set_border_width(12);

  port_entry = manage(new Gtk::SpinButton());
  attempts_entry = manage(new Gtk::SpinButton());
  interval_entry = manage(new Gtk::SpinButton());

  port_entry->set_range(1024, 65535);
  port_entry->set_increments(1, 10);
  port_entry->set_numeric(true);
  port_entry->set_width_chars(10);
  
  attempts_entry->set_range(0, 100);
  attempts_entry->set_increments(1, 10);
  attempts_entry->set_numeric(true);
  attempts_entry->set_width_chars(10);

  interval_entry->set_range(1, 3600);
  interval_entry->set_increments(1, 10);
  interval_entry->set_numeric(true);
  interval_entry->set_width_chars(10);

  advanced_frame->add(_("Server port:"), *port_entry);
  advanced_frame->add(_("Reconnect attempts:"), *attempts_entry);
  advanced_frame->add(_("Reconnect interval:"), *interval_entry);

  advanced_frame->set_border_width(12);
  tnotebook->append_page(*advanced_frame, _("Advanced"));

  port_entry->signal_changed().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_port_changed));
  interval_entry->signal_changed().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_interval_changed));
  attempts_entry->signal_changed().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_attempts_changed));
  
}


void
NetworkPreferencePage::create_peers_page(Gtk::Notebook *tnotebook)
{
  Gtk::VBox *gp = manage(new Gtk::VBox(false, 6));
  gp->set_border_width(6);

  // Info text
  const char *label =
    _("The following list specifies the hosts that Workrave connects to on\n"
      "start-up. Click the host name or port number to edit.");
    
  Gtk::HBox *infohbox = manage(new Gtk::HBox(false, 6));
  Gtk::Label *info_lab = manage(new Gtk::Label(label));

  infohbox->pack_start(*info_lab, false, false, 0);
  gp->pack_start(*infohbox, false, false, 0);
  
  
  //
  Gtk::HBox *hbox = manage(new Gtk::HBox(false, 6));

  peers_list = manage(new Gtk::TreeView());
  create_model();

  // create tree view
  peers_list->set_model(peers_store);
  peers_list->set_rules_hint();
  peers_list->set_search_column(peers_columns.hostname.index());

  Glib::RefPtr<Gtk::TreeSelection> selection = peers_list->get_selection();
  selection->set_mode(Gtk::SELECTION_MULTIPLE);

  Gtk::CellRendererText *renderer = NULL;
  Gtk::TreeViewColumn *column = NULL;
  int cols_count = 0;
  
  renderer = Gtk::manage(new Gtk::CellRendererText());
  cols_count = peers_list->append_column(_("Host name"), *renderer);
  column = peers_list->get_column(cols_count-1);
  column->add_attribute(renderer->property_text(), peers_columns.hostname);
  column->set_resizable(true);
  renderer->property_editable().set_value(true);
  renderer->signal_edited().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_hostname_edited));
    
  renderer = Gtk::manage(new Gtk::CellRendererText());
  cols_count = peers_list->append_column(_("Port"), *renderer);
  column = peers_list->get_column(cols_count-1);
  column->add_attribute(renderer->property_text(), peers_columns.port);
  column->set_resizable(true);
  renderer->property_editable().set_value(true);
  renderer->signal_edited().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_port_edited));

  Gtk::ScrolledWindow *peers_scroll = manage(new Gtk::ScrolledWindow());
  peers_scroll->add(*peers_list);
  
  Gtk::VBox *peersvbox = manage(new Gtk::VBox(true, 6));
  peersvbox->pack_start(*peers_scroll, true, true, 0);

  hbox->pack_start(*peersvbox, true, true, 0);

  peers_store->signal_row_changed().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_row_changed));
  peers_store->signal_row_inserted().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_row_changed));
  peers_store->signal_row_deleted().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_row_deleted));

                                    
  // Buttons
  remove_btn = manage(new Gtk::Button(Gtk::Stock::REMOVE));
  remove_btn->signal_clicked().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_peer_remove));
  add_btn = manage(new Gtk::Button(Gtk::Stock::ADD));
  add_btn->signal_clicked().connect(MEMBER_SLOT(*this, &NetworkPreferencePage::on_peer_add));

  Gtk::VBox *btnbox= manage(new Gtk::VBox(false, 6));
  btnbox->pack_start(*add_btn, false, false, 0);
  btnbox->pack_start(*remove_btn, false, false, 0);
  
  hbox->pack_start(*btnbox, false, false, 0);
  
  gp->pack_start(*hbox, true, true, 0);
  
  gp->show_all();
  
  gp->set_border_width(12);
  tnotebook->append_page(*gp, _("Hosts"));
}

void
NetworkPreferencePage::create_model()
{
  peers_store = Gtk::ListStore::create(peers_columns);

  list<string> peers = dist_manager->get_peers();

  for (list<string>::iterator i = peers.begin(); i != peers.end(); i++)
    {
      string peer = *i;
      string hostname, port;

      if (peer != "")
        {
          std::string::size_type pos = peer.find("tcp://");
          if (pos != std::string::npos)
            {
              hostname = peer.substr(6);
                  
              pos = hostname.rfind(":");
              if (pos != std::string::npos)
                {
                  port = hostname.substr(pos + 1);
                  hostname = hostname.substr(0, pos);
                }
                  
              Gtk::TreeRow row = *(peers_store->append());
                  
              row[peers_columns.hostname]  = hostname;
              row[peers_columns.port]      = port;
            }
        }
    }

}
  

void
NetworkPreferencePage::init_page_values()
{
  // Master enabled switch.
  bool enabled = dist_manager->get_enabled();
  enabled_cb->set_active(enabled);

  // Username.
  string str = dist_manager->get_username();
  username_entry->set_text(str);

  // Password
  str = dist_manager->get_password();
  password_entry->set_text(str);

  // Port
  int value = dist_manager->get_port();
  port_entry->set_value(value);

  // Attempts
  value = dist_manager->get_reconnect_attempts();
  attempts_entry->set_value(value);

  // Interval
  value = dist_manager->get_reconnect_interval();
  interval_entry->set_value(value);

}


void
NetworkPreferencePage::on_enabled_toggled()
{
  bool enabled = enabled_cb->get_active();
  dist_manager->set_enabled(enabled);
}


void
NetworkPreferencePage::on_username_changed()
{
  string name = username_entry->get_text();
  dist_manager->set_username(name);
}


void
NetworkPreferencePage::on_password_changed()
{
  string pw = password_entry->get_text();
  dist_manager->set_password(pw);
}


void
NetworkPreferencePage::on_port_changed()
{
  int value = (int) port_entry->get_value();
  dist_manager->set_port(value);
}


void
NetworkPreferencePage::on_interval_changed()
{
  int value = (int) interval_entry->get_value();
  dist_manager->set_reconnect_interval(value);
}


void
NetworkPreferencePage::on_attempts_changed()
{
  int value = (int) attempts_entry->get_value();
  dist_manager->set_reconnect_attempts(value);
}

void
NetworkPreferencePage::on_peer_remove()
{
  TRACE_ENTER("NetworkPreferencePage::on_peer_remove");
  Glib::RefPtr<Gtk::TreeSelection> selection = peers_list->get_selection();

#ifdef HAVE_GTKMM24
  const Gtk::TreeSelection::SlotForeachIter& slot =
    sigc::mem_fun(*this, &NetworkPreferencePage::remove_peer);

  selection->selected_foreach_iter(slot);
#else
  selection->selected_foreach(SigC::slot(*this, &NetworkPreferencePage::remove_peer));
#endif
  
  Glib::RefPtr<Gtk::ListStore> new_store = Gtk::ListStore::create(peers_columns);

  typedef Gtk::TreeModel::Children type_children;
  type_children children = peers_store->children();
  for (type_children::iterator iter = children.begin(); iter != children.end(); ++iter)
    {
      Gtk::TreeModel::Row row = *iter;
      
      string hostname = row[peers_columns.hostname];
      string port = row[peers_columns.port];

      if (hostname != "" || port != "")
        {
          Gtk::TreeRow new_row = *(new_store->append());
          
          new_row[peers_columns.hostname]  = hostname;
          new_row[peers_columns.port]      = port;
        }
    }

  peers_store = new_store;
  peers_list->set_model(peers_store);
  TRACE_EXIT();
}


void
NetworkPreferencePage::on_peer_add()
{
  Gtk::TreeRow row = *(peers_store->append());
  int port = (int) port_entry->get_value();
  
  stringstream ss;
  ss << port;
  
  row[peers_columns.hostname]  = "";
  row[peers_columns.port]      = ss.str();
}

void
NetworkPreferencePage::remove_peer(const Gtk::TreeModel::iterator &iter)
{
  Gtk::TreeModel::Row row = *iter;
  string s = row[peers_columns.hostname];
  row[peers_columns.hostname]  = "";
  row[peers_columns.port]      = "";
}

void
NetworkPreferencePage::on_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
{
  (void) path;
  (void) iter;
  update_peers();
}


void
NetworkPreferencePage::on_row_deleted(const Gtk::TreeModel::Path& path)
{
  (void) path;
  update_peers();
}


void
NetworkPreferencePage::on_hostname_edited(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
  GtkTreePath *gpath = gtk_tree_path_new_from_string (path_string.c_str());
  Gtk::TreePath path(gpath);

  /* get toggled iter */
  Gtk::TreeRow row = *(peers_store->get_iter(path));

  row[peers_columns.hostname]  = new_text;
}


void
NetworkPreferencePage::on_port_edited(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
  GtkTreePath *gpath = gtk_tree_path_new_from_string (path_string.c_str());
  Gtk::TreePath path(gpath);

  /* get toggled iter */
  Gtk::TreeRow row = *(peers_store->get_iter(path));

  row[peers_columns.port]  = new_text;
}


void
NetworkPreferencePage::update_peers()
{
  string peers;
  bool first = true;
  
  typedef Gtk::TreeModel::Children type_children;
  type_children children = peers_store->children();
  for (type_children::iterator iter = children.begin(); iter != children.end(); ++iter)
    {
      Gtk::TreeModel::Row row = *iter;

      string hostname = row[peers_columns.hostname];
      string port = row[peers_columns.port];

      if (!first)
        {
          peers += ",";
        }

      if (hostname != "" && port != "")
        {
          peers += "tcp://" + hostname + ":" + port;
          first = false;
        }
    }

  dist_manager->set_peers(peers);
}

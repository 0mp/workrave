// CollectivePreferencePage.cc --- Preferences widgets for a timer
//
// Copyright (C) 2002 Rob Caelers & Raymond Penners
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

#include "preinclude.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"

#include <sstream>

#include <unistd.h>
#include "GUIControl.hh"
#include "CollectivePreferencePage.hh"
#include "Configurator.hh"
#include "DistributionManager.hh"
#include "DistributionSocketLink.hh"

using std::cout;
using SigC::slot;


CollectivePreferencePage::CollectivePreferencePage()
  : Gtk::HBox(false, 6)
{
  TRACE_ENTER("CollectivePreferencePage::CollectivePreferencePage");

  Gtk::Notebook *tnotebook = manage(new Gtk::Notebook());
  tnotebook->set_tab_pos(Gtk::POS_TOP);  

  create_general_page(tnotebook);
  create_peers_page(tnotebook);
  create_advanced_page(tnotebook);

  init_page_values();
  
  tnotebook->show_all();

  pack_start(*tnotebook, true, true, 0);

  TRACE_EXIT();
}


CollectivePreferencePage::~CollectivePreferencePage()
{
  TRACE_ENTER("CollectivePreferencePage::~CollectivePreferencePage");
  TRACE_EXIT();
}


void
CollectivePreferencePage::create_general_page(Gtk::Notebook *tnotebook)
{
  Gtk::HBox *box = manage(new Gtk::HBox(false, 3));
  Gtk::Label *lab = manage(new Gtk::Label("General"));
  // Gtk::Image *img = manage(new Gtk::Image();
  // box->pack_start(*img, false, false, 0);
  box->pack_start(*lab, false, false, 0);

  Gtk::VBox *gp = manage(new Gtk::VBox(false, 6));
  gp->set_border_width(6);

  // Main switch
  enabled_cb = manage(new Gtk::CheckButton("Enable collective"));

  // Identity
  Gtk::Frame *id_frame = new Gtk::Frame("Identity");

  username_entry = manage(new Gtk::Entry());
  password_entry = manage(new Gtk::Entry());
  
  Gtk::Label *username_label = manage(new Gtk::Label("Username"));
  Gtk::Label *password_label = manage(new Gtk::Label("Password"));

  password_entry->set_visibility(false);
  password_entry->set_invisible_char('*');
  
  Gtk::Table *id_table = manage(new Gtk::Table(3, 2, false));
  id_table->set_row_spacings(2);
  id_table->set_col_spacings(6);
  id_table->set_border_width(6);
  int y = 0;
  id_table->attach(*username_label, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  id_table->attach(*username_entry, 1, 2, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  id_table->attach(*password_label, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  id_table->attach(*password_entry, 1, 2, y, y+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
  y++;

  id_frame->add(*id_table);
  
  gp->pack_start(*enabled_cb, false, false, 0);
  gp->pack_start(*id_frame, false, false, 0);

  box->show_all();
  tnotebook->pages().push_back(Gtk::Notebook_Helpers::TabElem(*gp, *box));

  enabled_cb->signal_toggled().connect(SigC::slot(*this, &CollectivePreferencePage::on_enabled_toggled));
  username_entry->signal_changed().connect(SigC::slot(*this, &CollectivePreferencePage::on_username_changed));
  password_entry->signal_changed().connect(SigC::slot(*this, &CollectivePreferencePage::on_password_changed));
}


void
CollectivePreferencePage::create_advanced_page(Gtk::Notebook *tnotebook)
{
  Gtk::HBox *box = manage(new Gtk::HBox(false, 3));
  Gtk::Label *lab = manage(new Gtk::Label("Advanced"));
  // Gtk::Image *img = manage(new Gtk::Image();
  // box->pack_start(*img, false, false, 0);
  box->pack_start(*lab, false, false, 0);

  Gtk::VBox *gp = manage(new Gtk::VBox(false, 6));
  gp->set_border_width(6);

  Gtk::Frame *advanced_frame = new Gtk::Frame("Server settings");

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

  Gtk::Label *port_label = manage(new Gtk::Label("Server port"));
  Gtk::Label *attempts_label = manage(new Gtk::Label("Reconnect atttempts"));
  Gtk::Label *interval_label = manage(new Gtk::Label("Reconnect interval"));

  
  Gtk::Table *advanced_table = manage(new Gtk::Table(3, 2, false));
  advanced_table->set_row_spacings(2);
  advanced_table->set_col_spacings(6);
  advanced_table->set_border_width(6);
  int y = 0;
  advanced_table->attach(*port_label, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  advanced_table->attach(*port_entry, 1, 2, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  y++;
  advanced_table->attach(*attempts_label, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  advanced_table->attach(*attempts_entry, 1, 2, y, y+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);
  y++;
  advanced_table->attach(*interval_label, 0, 1, y, y+1, Gtk::SHRINK, Gtk::SHRINK);
  advanced_table->attach(*interval_entry, 1, 2, y, y+1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK);

  advanced_frame->add(*advanced_table);
  
  gp->pack_start(*advanced_frame, false, false, 0);

  box->show_all();
  tnotebook->pages().push_back(Gtk::Notebook_Helpers::TabElem(*gp, *box));

  port_entry->signal_changed().connect(SigC::slot(*this, &CollectivePreferencePage::on_port_changed));
  interval_entry->signal_changed().connect(SigC::slot(*this, &CollectivePreferencePage::on_interval_changed));
  attempts_entry->signal_changed().connect(SigC::slot(*this, &CollectivePreferencePage::on_attempts_changed));
  
}


void
CollectivePreferencePage::create_peers_page(Gtk::Notebook *tnotebook)
{
  Gtk::HBox *box = manage(new Gtk::HBox(false, 3));
  Gtk::Label *lab = manage(new Gtk::Label("Hosts"));
  // Gtk::Image *img = manage(new Gtk::Image();
  // box->pack_start(*img, false, false, 0);
  box->pack_start(*lab, false, false, 0);

  Gtk::VBox *gp = manage(new Gtk::VBox(false, 6));
  gp->set_border_width(6);

  // Info text
  const char *label =
    "The following list specifies the hosts that Workrave connects to on\n"
    "start-up. Click the host name or port number to edit.";
    
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
  cols_count = peers_list->append_column("Host name", *renderer);
  column = peers_list->get_column(cols_count-1);
  column->add_attribute(renderer->property_text(), peers_columns.hostname);
  column->set_resizable(true);
  renderer->property_editable().set_value(true);
  renderer->signal_edited().connect(SigC::slot(*this, &CollectivePreferencePage::on_hostname_edited));
    
  renderer = Gtk::manage(new Gtk::CellRendererText());
  cols_count = peers_list->append_column("Port", *renderer);
  column = peers_list->get_column(cols_count-1);
  column->add_attribute(renderer->property_text(), peers_columns.port);
  column->set_resizable(true);
  renderer->property_editable().set_value(true);
  renderer->signal_edited().connect(SigC::slot(*this, &CollectivePreferencePage::on_port_edited));

  Gtk::ScrolledWindow *peers_scroll = manage(new Gtk::ScrolledWindow());
  peers_scroll->add(*peers_list);
  
  Gtk::VBox *peersvbox = manage(new Gtk::VBox(true, 6));
  peersvbox->pack_start(*peers_scroll, true, true, 0);

  hbox->pack_start(*peersvbox, true, true, 0);

  peers_store->signal_row_changed().connect(SigC::slot(*this, &CollectivePreferencePage::on_row_changed));
  peers_store->signal_row_inserted().connect(SigC::slot(*this, &CollectivePreferencePage::on_row_changed));
  peers_store->signal_row_deleted().connect(SigC::slot(*this, &CollectivePreferencePage::on_row_deleted));

                                    
  // Buttons
  remove_btn = manage(new Gtk::Button(Gtk::Stock::REMOVE));
  remove_btn->signal_clicked().connect(SigC::slot(*this, &CollectivePreferencePage::on_peer_remove));
  add_btn = manage(new Gtk::Button(Gtk::Stock::ADD));
  add_btn->signal_clicked().connect(SigC::slot(*this, &CollectivePreferencePage::on_peer_add));

  Gtk::VBox *btnbox= manage(new Gtk::VBox(false, 6));
  btnbox->pack_start(*add_btn, false, false, 0);
  btnbox->pack_start(*remove_btn, false, false, 0);
  
  hbox->pack_start(*btnbox, false, false, 0);
  
  gp->pack_start(*hbox, true, true, 0);
  
  box->show_all();
  gp->show_all();
  
  tnotebook->pages().push_back(Gtk::Notebook_Helpers::TabElem(*gp, *box));
  
}

void
CollectivePreferencePage::create_model()
{
  peers_store = Gtk::ListStore::create(peers_columns);

  DistributionManager *dist_manager = DistributionManager::get_instance();
  if (dist_manager != NULL)
    {
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
}
  

void
CollectivePreferencePage::init_page_values()
{
  Configurator *c = GUIControl::get_instance()->get_configurator();
  bool is_set;

  // Master enabled switch.
  bool enabled = false;
  is_set = c->get_value(DistributionManager::CFG_KEY_DISTRIBUTION + DistributionManager::CFG_KEY_DISTRIBUTION_ENABLED, &enabled);
  if (!is_set)
    {
      enabled = false;
    }
  
  enabled_cb->set_active(enabled);

  // Username.
  string str;
  is_set = c->get_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_USERNAME, &str);
  if (!is_set)
    {
      str = "";
    }
  
  username_entry->set_text(str);

  // Password
  is_set = c->get_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_PASSWORD, &str);
  if (!is_set)
    {
      str = "";
    }
  
  password_entry->set_text(str);

  // Port
  int value;
  is_set = c->get_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_PORT, &value);
  if (!is_set)
    {
      value = DEFAULT_PORT;
    }
  
  port_entry->set_value(value);

  // Attempts
  is_set = c->get_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_ATTEMPTS, &value);
  if (!is_set)
    {
      value = DEFAULT_ATTEMPTS;
    }
  
  attempts_entry->set_value(value);

  // Interval
  is_set = c->get_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_INTERVAL, &value);
  if (!is_set)
    {
      value = DEFAULT_INTERVAL;
    }
  
  interval_entry->set_value(value);

  // Peers
  is_set = c->get_value(DistributionManager::CFG_KEY_DISTRIBUTION + DistributionManager::CFG_KEY_DISTRIBUTION_PEERS, &str);
  if (!is_set)
    {
      str = "";
    }

}


void
CollectivePreferencePage::on_enabled_toggled()
{
  TRACE_ENTER("CollectivePreferencePage::on_enabled_toggled");
  bool enabled = enabled_cb->get_active();
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(DistributionManager::CFG_KEY_DISTRIBUTION + DistributionManager::CFG_KEY_DISTRIBUTION_ENABLED, enabled);
  TRACE_EXIT();
}


void
CollectivePreferencePage::on_username_changed()
{
  string name = username_entry->get_text();
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_USERNAME, name);
}


void
CollectivePreferencePage::on_password_changed()
{
  string pw = password_entry->get_text();
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_PASSWORD, pw);
}


void
CollectivePreferencePage::on_port_changed()
{
  int value = (int) port_entry->get_value();
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_PORT, value);
}


void
CollectivePreferencePage::on_interval_changed()
{
  int value = (int) interval_entry->get_value();
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_INTERVAL, value);
}


void
CollectivePreferencePage::on_attempts_changed()
{
  int value = (int) attempts_entry->get_value();
  Configurator *c = GUIControl::get_instance()->get_configurator();
  c->set_value(DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP + DistributionSocketLink::CFG_KEY_DISTRIBUTION_TCP_ATTEMPTS, value);
}

void
CollectivePreferencePage::on_peer_remove()
{
  Glib::RefPtr<Gtk::TreeSelection> selection = peers_list->get_selection();

  selection->selected_foreach(SigC::slot(*this, &CollectivePreferencePage::remove_peer));
}


void
CollectivePreferencePage::on_peer_add()
{
  Gtk::TreeRow row = *(peers_store->append());
  int port = (int) port_entry->get_value();
  
  stringstream ss;
  ss << port;
  
  row[peers_columns.hostname]  = "";
  row[peers_columns.port]      = ss.str();
}

void
CollectivePreferencePage::remove_peer(const Gtk::TreeModel::iterator &iter)
{
  Gtk::TreeModel::Row row = *iter;
  row[peers_columns.hostname]  = "";
  row[peers_columns.port]      = "";

  //peers_store->erase(iter);
}


void
CollectivePreferencePage::on_row_changed(const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter)
{
  update_peers();
}


void
CollectivePreferencePage::on_row_deleted(const Gtk::TreeModel::Path& path)
{
  update_peers();
}

void
CollectivePreferencePage::on_hostname_edited(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
  GtkTreePath *gpath = gtk_tree_path_new_from_string (path_string.c_str());
  Gtk::TreePath path(gpath);

  /* get toggled iter */
  Gtk::TreeRow row = *(peers_store->get_iter(path));

  row[peers_columns.hostname]  = new_text;
}


void
CollectivePreferencePage::on_port_edited(const Glib::ustring& path_string, const Glib::ustring& new_text)
{
  GtkTreePath *gpath = gtk_tree_path_new_from_string (path_string.c_str());
  Gtk::TreePath path(gpath);

  /* get toggled iter */
  Gtk::TreeRow row = *(peers_store->get_iter(path));

  row[peers_columns.port]  = new_text;
}


void
CollectivePreferencePage::update_peers()
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

  DistributionManager *dist_manager = DistributionManager::get_instance();
  if (dist_manager != NULL)
    {
      dist_manager->set_peers(peers);
    }
}

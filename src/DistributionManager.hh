// DistributionManager.hh
//
// Copyright (C) 2002, 2003 Rob Caelers <robc@krandor.org>
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

#ifndef DISTRIBUTIOMANAGER_HH
#define DISTRIBUTIOMANAGER_HH

#include <string>
#include <list>

using namespace std;

#include "ConfiguratorListener.hh"
#include "DistributionLinkListener.hh"
#include "DistributionClientMessageInterface.hh"

class DistributionLink;
class Configurator;
class DistributionLogListener;
class DistributionListener;

class DistributionManager :
  public DistributionLinkListener,
  public ConfiguratorListener
{
public:
  static const string CFG_KEY_DISTRIBUTION;
  static const string CFG_KEY_DISTRIBUTION_ENABLED;
  static const string CFG_KEY_DISTRIBUTION_PEERS;

  enum NodeState { NODE_ACTIVE, NODE_PASSIVE, NODE_STANDBY };
  
  static DistributionManager *get_instance();
  
  DistributionManager();
  virtual ~DistributionManager();

  NodeState get_state() const;
  void init(Configurator *conf);
  void heartbeart();
  bool is_master() const;
  string get_master_id() const;
  string get_my_id() const;
  int get_number_of_peers();
  bool claim();
  bool set_lock_master(bool lock);
  bool join(string url);
  bool register_client_message(DistributionClientMessageID id, DistributionClientMessageType type,
                               DistributionClientMessageInterface *dist_state);
  bool unregister_client_message(DistributionClientMessageID id);

  bool add_listener(DistributionListener *listener);
  bool remove_listener(DistributionListener *listener);
  
  bool broadcast_client_message(DistributionClientMessageID id, unsigned char *buffer, int size);
  bool add_peer(string peer);
  bool remove_peer(string peer);
  bool disconnect_all();
  bool reconnect_all();
  void set_peers(string peers, bool connect = true);
  list<string> get_peers() const;
  
  // Logging.
  bool add_log_listener(DistributionLogListener *listener);
  bool remove_log_listener(DistributionLogListener *listener);
  list<string> get_logs() const;
  
private:
  void sanitize_peer(string &peer);
  void parse_peers(string peers, bool connect = true);
  void write_peers();
  void read_configuration();
  void config_changed_notify(string key);

  void fire_log_event(string message);
  void fire_signon_client(char *id);
  void fire_signoff_client(char *id);

  // DistributionLinkListener
  void master_changed(bool result, char *id);
  void signon_remote_client(char *client_id);
  void signoff_remote_client(char *client_id);
  void log(char *fmt, ...);
  
private:
  typedef std::list<DistributionLogListener *> LogListeners;
  typedef std::list<DistributionLogListener *>::iterator LogListenerIter;

  typedef std::list<DistributionListener *> Listeners;
  typedef std::list<DistributionListener *>::iterator ListenerIter;
  
  //! The one and only instance
  static DistributionManager *instance;

  //! Is distribution operation enabled?
  bool distribution_enabled;

  //! Access to the configuration.
  Configurator *configurator;
  
  //! Link to other clients
  DistributionLink *link;

  //! Current State
  NodeState state;

  //! All peers
  list<string> peer_urls;

  // ! All log messages
  list<string> log_messages;

  //! Log listeners.
  LogListeners log_listeners;

  //! Event listeners.
  Listeners listeners;

  //! Current master.
  string current_master;
};


//! Returns the singleton distribution manager.
inline DistributionManager *
DistributionManager::get_instance()
{
  if (instance == NULL)
    {
      instance = new DistributionManager;
    }
  return instance;
}


//! Returns log messages.
inline list<string>
DistributionManager::get_logs() const
{
  return log_messages;
}


//! Returns all peers.
inline list<string>
DistributionManager::get_peers() const
{
  return peer_urls;
}

#endif // DISTRIBUTIOMANAGER_HH

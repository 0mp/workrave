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
#include "DistributedStateInterface.hh"

class DistributionLink;
class Configurator;
class DistributionLogListener;

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
  int get_number_of_peers();
  bool claim();
  bool set_lock_master(bool lock);
  bool join(string url);
  bool register_state(DistributedStateID id, DistributedStateInterface *dist_state);
  bool unregister_state(DistributedStateID id);
  bool push_state(DistributedStateID id, unsigned char *buffer, int size);
  bool add_peer(string peer);
  bool remove_peer(string peer);
  bool disconnect_all();
  bool reconnect_all();
  void set_peers(string peers, bool connect = true);
  
  //
  void master_changed(bool result);
  void state_transfer_complete();
  void log(char *fmt, ...);

  list<string> get_peers()
  {
    return peer_urls;
  }

  bool add_log_listener(DistributionLogListener *listener);
  bool remove_log_listener(DistributionLogListener *listener);
  void fire_event(string message);
  list<string> get_logs() const
  {
    return log_messages;
  }
  
private:
  void sanitize_peer(string &peer);
  void parse_peers(string peers, bool connect = true);
  void write_peers();
  void read_configuration();
  void config_changed_notify(string key);
  
private:
  typedef std::list<DistributionLogListener *> Listeners;
  typedef std::list<DistributionLogListener *>::iterator ListenerIter;
  
  //! Is distribution operation enabled?
  bool distribution_enabled;

  //! Did we received all state after becoming master?
  bool state_complete;
  
  //! The one and only instance
  static DistributionManager *instance;

  //! Access to the configuration.
  Configurator *configurator;
  
  //! Link to other clients
  DistributionLink *link;

  //! State
  NodeState state;

  //! All peers
  list<string> peer_urls;

  // ! All log messages
  list<string> log_messages;

  //! Log listeners.
  Listeners listeners;
};



inline DistributionManager *
DistributionManager::get_instance()
{
  if (instance == NULL)
    {
      instance = new DistributionManager;
    }
  return instance;
}

#endif // DISTRIBUTIOMANAGER_HH

// DistributionManager.hh
//
// Copyright (C) 2002 Rob Caelers <robc@krandor.org>
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

#include "DistributionLinkListener.hh"

class DistributionLink;
class DistributedStateInterface;

class DistributionManager : public DistributionLinkListener
{
public:
  enum NodeState { NODE_ACTIVE, NODE_PASSIVE, NODE_STANDBY };
  enum DistributesStateID
    {
      DISTR_STATE_TIMER_MP = 0x0010,
      DISTR_STATE_TIMER_RB = 0x0011,
      DISTR_STATE_TIMER_DL = 0x0012,
    };

  
  static DistributionManager *get_instance();
  
  DistributionManager();
  virtual ~DistributionManager();

  NodeState get_state() const;
  bool is_active() const;
  int get_number_of_peers();
  bool claim();
  bool join(string url);
  bool register_state(DistributesStateID id, DistributedStateInterface *dist_state);
  void unregister_state(int state_id);

  //
  virtual void active_changed(bool result);
  
private:
  //! The one and only instance
  static DistributionManager *instance;

  //! Link to other clients
  DistributionLink *link;

  //! State
  NodeState state;
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

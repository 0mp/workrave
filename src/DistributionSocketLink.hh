// DistributionSocketLink.hh
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

#ifndef DISTRIBUTIONSOCKETLINK_HH
#define DISTRIBUTIONSOCKETLINK_HH

#include <list>
#include <map>
#include <glib.h>
#include <gnet/gnet.h>

#include "DistributionLink.hh"
#include "DistributedStateInterface.hh"
#include "PacketBuffer.hh"

class DistributionLinkListener;

class DistributionSocketLink : public DistributionLink
{
private:
  enum PacketCommand {
    PACKET_HELLO 	= 0x0001,
    PACKET_CLAIM 	= 0x0002,
    PACKET_CLIENT_LIST	= 0x0003,
    PACKET_WELCOME	= 0x0004,
    PACKET_NEW_MASTER	= 0x0005,
    PACKET_STATEINFO	= 0x0006,
  };

  enum ClientListFlags
    {
      CLIENTLIST_FORWARDABLE  		= 1,
      CLIENTLIST_IAM_ACTIVE   		= 2,
      CLIENTLIST_HAS_ACTIVE_REF   	= 4,
    };
  
  struct Client
  {
    Client() :
      canonical_name(NULL),
      server_name(NULL),
      server_port(0),
      socket(NULL),
      iochannel(NULL),
      watch_flags(0),
      watch(0),
      link(NULL)
    {
    }

    ~Client()
    {
      if (canonical_name != NULL)
        {
          g_free(canonical_name);
        }
      if (server_name != NULL)
        {
          g_free(server_name);
        }

      if (iochannel != NULL)
        {
          g_io_channel_unref(iochannel);
        }

      if (socket != NULL)
        {
          gnet_tcp_socket_delete(socket);
        }
      
      g_source_remove(watch);
    }
    
    //! Canonical IP.
    gchar *canonical_name;
    
    //! Hostname/IP of this client.
    gchar *server_name;

    //! Local port.
    gint server_port;

    //! GNet socket
    GTcpSocket *socket;

    //! Glib IOChannel.
    GIOChannel *iochannel;

    //! I/O Events we are monitoring.
    gint watch_flags;

    //! Our Watch
    guint watch;

    //! For statics...
    DistributionSocketLink *link;

    //!
    PacketBuffer packet;
  };

  
public:
  DistributionSocketLink();
  virtual ~DistributionSocketLink();
  
  int get_number_of_peers();
  void set_distribution_manager(DistributionLinkListener *dll);
  bool init(gint port);
  void set_user(string user, string password);
  void join(string url);
  bool claim();

  bool register_state(DistributedStateID id, DistributedStateInterface *dist_state);
  bool unregister_state(DistributedStateID id);
  
private:
  bool add_client(gchar *host, gint port);
  bool remove_client(Client *client);
  Client *find_client_by_servername(gchar *name, gint port);
  Client *find_client_by_canonicalname(gchar *name, gint port);

  void set_active(gchar *cname, gint port);
  void set_active(Client *client);
  void set_me_active();

  
  void init_packet(PacketBuffer &packet, PacketCommand cmd);
  void send_packet_broadcast(PacketBuffer &packet);
  void send_packet_except(PacketBuffer &packet, Client *client);
  void send_packet(Client *client, PacketBuffer &packet);

  void process_client_packet(Client *client);

  void handle_hello(Client *client);
  void handle_welcome(Client *client);
  void handle_client_list(Client *client);
  void handle_claim(Client *client);
  void handle_new_master(Client *client);
  void handle_state(Client *client);
  
  void send_hello(Client *client);
  void send_welcome(Client *client);
  void send_client_list(Client *client);
  void send_claim(Client *client);
  void send_new_master(Client *client = NULL);
  void send_state();
  
  bool start_async_server();
  void async_accept(GTcpSocket *server, GTcpSocket *client);
  bool async_server_io(GIOChannel* iochannel, GIOCondition condition, Client *client);
  void async_client_connfunc(GTcpSocket *socket, GInetAddr *ia, GTcpSocketConnectAsyncStatus status, Client *client);

  static void static_async_accept(GTcpSocket* server, GTcpSocket* client, gpointer data);
  static gboolean static_async_server_io(GIOChannel* iochannel, GIOCondition condition, gpointer data);
  static void static_async_client_connfunc(GTcpSocket *socket, GInetAddr *ia, GTcpSocketConnectAsyncStatus status, gpointer data);

  
private:
  //! Username for client authenication
  gchar *username;

  //! Password for client authenication.
  gchar *password;

  //! All clients.
  list<Client *> clients;

  //! Active client
  Client *active_client;

  //!
  bool active;
  
  //!
  gchar *canonical_name;
  
  //! My server port
  gint server_port;
  
  //! The server socket.
  GTcpSocket *server_socket;

  //!
  DistributionLinkListener *dist_manager;

  //! State
  map<DistributedStateID, DistributedStateInterface *> state_map;
};

#endif // DISTRIBUTIONSOCKETLINK_HH

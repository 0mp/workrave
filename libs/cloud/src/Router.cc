// Router.cc
//
// Copyright (C) 2007, 2008, 2009, 2012 Rob Caelers <robc@krandor.nl>
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
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#define TRACE_EXTRA " (" << myid.str() << ")"
#include "debug.hh"

#include <string.h>
#include <string>
#include <sstream>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <glib.h>

#include "Router.hh"
#include "Util.hh"

#include "cloud.pb.h"

using namespace std;
using namespace workrave::cloud;

//! Create a new network router
Router::Ptr
Router::create()
{
  return Router::Ptr(new Router());
}

// TODO: move to factory
//! Create a new network router
ICloud::Ptr
ICloud::create()
{
  return Router::create();
}


//! Constructs a new network router
Router::Router()
{
  TRACE_ENTER("Router::Router");
  marshaller = Marshaller::create();
  announce = Announce::create(marshaller);
  direct_link_manager = DirectLinkManager::create(marshaller);
  TRACE_EXIT();
}


//! Destructs the network router.
Router::~Router()
{
  TRACE_ENTER("Router::~Router");
  TRACE_EXIT();
}


//! Initializes the network router.
void
Router::init(int port, string username, string secret)
{
  TRACE_ENTER("Router::init");
  this->username = username;
  this->secret = secret;
  
  init_myid(port);

  marshaller->set_id(myid);
  marshaller->set_credentials(username, secret);
  
  announce->init(port);
  direct_link_manager->init(port);

  announce->signal_data().connect(boost::bind(&Router::on_data, this, _1, _2, SCOPE_MULTICAST));
  direct_link_manager->signal_new_link().connect(boost::bind(&Router::on_direct_link_created, this, _1));

  TRACE_EXIT();
}


//! Terminates the network router.
void
Router::terminate()
{
  TRACE_ENTER("Router::terminate");
  TRACE_EXIT();
}


void
Router::connect(const string &host, int port)
{
  TRACE_ENTER_MSG("Router::connect", host << " " << port);

  Client::Ptr client = Client::create();
  
  DirectLink::Ptr link = DirectLink::create(marshaller);
  link->connect(host, port);

  link->signal_data().connect(boost::bind(&Router::on_data, this, _1, _2, SCOPE_DIRECT));
  link->signal_state().connect(boost::bind(&Router::on_direct_link_state_changed, this, _1));
  
  client->link = link;
  
  clients.push_back(client);

  TRACE_EXIT();
}


void
Router::send_message(Message::Ptr message, MessageParams::Ptr params)
{
  TRACE_ENTER("Router::send_message");

  PacketOut::Ptr packet = PacketOut::create(message);
  packet->sign = params->sign;
  packet->source = myid;
  
  string msg_str = marshaller->marshall(packet);

  Scope scope = params->scope;
  
  if ((scope & SCOPE_DIRECT) == SCOPE_DIRECT)
    {
      for (ClientIter it = clients.begin(); it != clients.end(); it++)
        {
          TRACE_MSG((*it)->link->state);
          if ((*it)->link->state == Link::CONNECTION_STATE_CONNECTED)
            {
              (*it)->link->send_message(msg_str);
            }
        }
    }
  
  if ((scope & SCOPE_MULTICAST) == SCOPE_MULTICAST)
    {
      announce->send_message(msg_str);
    }
      
  TRACE_EXIT();
}


void
Router::forward_message(Link::Ptr link, PacketIn::Ptr packet)
{
  TRACE_ENTER("Router::forward_message");

  PacketOut::Ptr packet_out(PacketOut::create(packet->message));
  packet_out->sign = packet->authentic;
  packet_out->source = packet->source;
  
  string msg_str = marshaller->marshall(packet_out);

  for (ClientIter it = clients.begin(); it != clients.end(); it++)
    {
      if ((*it)->link->state == Link::CONNECTION_STATE_CONNECTED &&
          (*it)->link != link)
        {
          (*it)->link->send_message(msg_str);
        }
    }

  TRACE_EXIT();
}

Router::MessageSignal &
Router::signal_message(int domain, int id)
{
  if (message_signals[std::make_pair(domain, id)] == NULL)
    {
      message_signals[std::make_pair(domain, id)].reset(new MessageSignal);
    }
  return *message_signals[std::make_pair(domain, id)].get();
}


bool
Router::on_data(Link::Ptr link, PacketIn::Ptr packet, Scope scope)
{
  TRACE_ENTER("Router::on_data");
  bool ret = true;
  
  if (packet)
    {
      TRACE_MSG("Source " << packet->source.str());
      //TRACE_MSG("Link " << link->id.str());

      if (myid == packet->source)
        {
          TRACE_MSG("cycle!");
          ret = false;
        }

      if (ret && packet->authentic)
        {
          MessageContext::Ptr context(MessageContext::create());
          context->source = packet->source;
          context->scope = scope;
          
          fire_message_signal(packet->header->domain(), packet->header->payload(), packet->message, context);
          
          if (packet->header->domain() == 0 && packet->header->payload() == proto::Alive::kTypeFieldNumber)
            {
              process_alive(link, packet);
            }
          else
            {
              forward_message(link, packet);
            }
        }
    }
  TRACE_RETURN(ret);
  return ret;
}


void
Router::on_direct_link_created(DirectLink::Ptr link)
{
  TRACE_ENTER("Router::on_direct_link_created");
  TRACE_MSG(link->address->str());

  Client::Ptr client = Client::create();
  
  link->signal_data().connect(boost::bind(&Router::on_data, this, _1, _2, SCOPE_DIRECT));
  link->signal_state().connect(boost::bind(&Router::on_direct_link_state_changed, this, _1));

  client->link = link;
  
  clients.push_back(client);
  send_alive();
  
  TRACE_EXIT();
}


void
Router::on_direct_link_state_changed(DirectLink::Ptr link)
{
  TRACE_ENTER("Router::on_direct_link_state_changed");

  for (ClientIter it = clients.begin(); it != clients.end(); it++)
    {
      if ((*it)->link == link)
        {
          if (link->state == Link::CONNECTION_STATE_CLOSED && it != clients.end())
            {
              clients.erase(it);
              TRACE_MSG("closed");
            }
          break;
        }
    }
  
  TRACE_EXIT();
}

void
Router::init_myid(int instanceid)
{
  TRACE_ENTER("::init_myid");
  bool ok = false;
  stringstream ss;

  ss << Util::get_home_directory() << "id-" << instanceid << ends;
  string idfilename = ss.str();

  if (Util::file_exists(idfilename))
    {
      ifstream file(idfilename.c_str());
      
      if (file)
        {
          string id_str;
          file >> id_str;

          myid = UUID::from_str(id_str);              

          file.close();
        }
    }

  if (! ok)
    {
      ofstream file(idfilename.c_str());

      file << myid.str() << endl;
      file.close();
    }


  TRACE_EXIT();
}


void
Router::fire_message_signal(int domain, int id, Message::Ptr message, MessageContext::Ptr context)
{
  MessageSignalMapIter it = message_signals.find(std::make_pair(domain, id));
  if (it != message_signals.end())
    {
      MessageSignal &m = *it->second;
      m(message, context);
    }
}


void
Router::send_alive()
{
  TRACE_ENTER("Router::send_alive");
  boost::shared_ptr<proto::Alive> a(new proto::Alive());
  a->set_hi("yo");

  string *c = a->add_path();
  *c = myid.raw();

  send_message(a, MessageParams::create());
  TRACE_EXIT();
}


Client::Ptr
Router::find_client(Link::Ptr link)
{
  TRACE_ENTER("Router::find_client");
  for (ClientIter it = clients.begin(); it != clients.end(); it++)
    {
      NetworkAddress::Ptr a = (*it)->link->address;
      if (a) {
        TRACE_MSG("cl " << a->str());
      }
      if ((*it)->link == link)
        {
          return *it;
        }
    }
  TRACE_EXIT();
  return Client::Ptr();
}


void
Router::process_alive(Link::Ptr link, PacketIn::Ptr packet)
{
  TRACE_ENTER("Router::process_alive");
  
  boost::shared_ptr<proto::Alive> a = boost::dynamic_pointer_cast<proto::Alive>(packet->message);

  if (a)
    {
      google::protobuf::RepeatedPtrField<string> path = a->path();

      TRACE_MSG("Source " << packet->source.str());
      for (google::protobuf::RepeatedPtrField<string>::iterator i = path.begin(); i != path.end(); i++)
        {
          TRACE_MSG("Path " << UUID::from_raw(*i).str());
        }

      Client::Ptr client = find_client(link);
      if (client)
        {
          TRACE_MSG("have client " << UUID::from_raw(a->path(a->path_size() - 1)).str());
              
          if (!client->authenticated)
            {
              client->authenticated = true;
              client->id = UUID::from_raw(a->path(a->path_size() - 1));
            }
          else
            {
              if (client->id != UUID::from_raw(a->path(a->path_size() - 1)))
                {
                  TRACE_MSG("Illegal change of ID ");
                }
            }
        }
      else
        {
          ViaLink::Ptr info = ViaLink::create(link);
          info->state = Link::CONNECTION_STATE_CONNECTED;
          info->address = link->address;
          info->via = link;
          TRACE_MSG("indirect");
        }

      string *c = a->add_path();
      *c = myid.raw();
      forward_message(link, packet);
    }
  TRACE_EXIT();
}

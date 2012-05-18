// NetworkMessage.hh
//
// Copyright (C) 2012 Rob Caelers <robc@krandor.nl>
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

#ifndef NETWORKMESSAGE_HH
#define NETWORKMESSAGE_HH

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "workrave.pb.h"

#include "NetworkClient.hh"
#include "UUID.hh"

template<class T>
class NetworkMessage;

class NetworkMessageBase : public boost::noncopyable
{
public:
  typedef boost::shared_ptr<NetworkMessageBase> Ptr;
  typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;

  NetworkMessageBase()
  {
  }

  NetworkMessageBase(MessagePtr message) : message(message)
  {
  }
  
  virtual ~NetworkMessageBase() {}
  
  static Ptr create(boost::shared_ptr<workrave::Header> header, MessagePtr message);
  
  MessagePtr msg()
  {
    return message;
  }
  
  template<class T>
  boost::shared_ptr<T> as()
  {
    return boost::dynamic_pointer_cast<T>(message);
  }
  
public:
  MessagePtr message;
  NetworkClient::Scope scope;
  bool authenticated;
  UUID source;
};


template<>
class NetworkMessage<google::protobuf::Message> : public NetworkMessageBase
{
public:
  typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;

  explicit NetworkMessage(MessagePtr message) : NetworkMessageBase(message)
  {
  }
};


template<class T>
class NetworkMessage : public NetworkMessage<google::protobuf::Message>
{
public:
  typedef boost::shared_ptr<NetworkMessage<T> > Ptr;
  typedef boost::shared_ptr<T> MessagePtr;

  explicit NetworkMessage(MessagePtr message) : NetworkMessage<google::protobuf::Message>(message)
  {
  }

  MessagePtr msg()
  {
    return boost::dynamic_pointer_cast<T>(message);
  }

  static Ptr create()
  {
    MessagePtr m(new T());
    Ptr ret = Ptr(new NetworkMessage<T>(m));
    return ret;
  }
};


#endif // NETWORKMESSAGE_HH

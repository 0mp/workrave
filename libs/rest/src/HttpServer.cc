// Copyright (C) 2010 - 2013 by Rob Caelers <robc@krandor.nl>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "HttpServer.hh"

#include "rest/AuthError.hh"

using namespace std;
using namespace workrave::rest;

HttpServer::Ptr
HttpServer::create(const std::string &user_agent, const std::string &path, IHttpServer::RequestCallback callback)
{
  return Ptr(new HttpServer(user_agent, path, callback));
}


HttpServer::HttpServer(const std::string &user_agent, const std::string &path, IHttpServer::RequestCallback callback)
  : user_agent(user_agent), path(path), callback(callback), server(NULL), port(0)
{
}


HttpServer::~HttpServer()
{
  stop();
  if (server)
    {
      g_object_unref(server);
    }
}


void
HttpServer::start()
{
  if (!server)
    {
      SoupAddress *addr = soup_address_new("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
      soup_address_resolve_sync(addr, NULL);

      // TODO: server header != user agent
      server = soup_server_new(SOUP_SERVER_SERVER_HEADER, user_agent.c_str(),
                               SOUP_SERVER_INTERFACE, addr,
                               NULL);
      g_object_unref (addr);

      if (server == NULL)
        {
          throw AuthError(AuthErrorCode::System, "Cannot receive incoming connections.");
        }
  
      port = soup_server_get_port(server);
      g_debug("Listening on %d", port);

      soup_server_add_handler(server, path.c_str(), server_callback_static, this, NULL);
    }
	soup_server_run_async(server);
}


void
HttpServer::stop()
{
  if (server != NULL)
    {
      soup_server_quit(server);
    }
}


void
HttpServer::server_callback_static(SoupServer *server, SoupMessage *message, const char *path,
                                       GHashTable *query, SoupClientContext *context, gpointer data)
{
  HttpServer *self = (HttpServer *)data;
  self->server_callback(server, message, path, query, context);
}


void
HttpServer::server_callback(SoupServer *, SoupMessage *message, const char *path,
                                GHashTable *query, SoupClientContext *context)
{
  (void) path;
  (void) context;
  (void) query;

  SoupURI *uri = soup_message_get_uri(message);

  IHttpRequest::Ptr request = IHttpRequest::create();
  request->uri = (uri != NULL && uri->query != NULL) ? uri->query : "";
  request->method = message->method;
  request->body = (message->response_body->length > 0) ? message->response_body->data : "";
  request->content_type = "";
  
  IHttpReply::Ptr reply = callback(request);

  for (map<string, string>::const_iterator i = reply->headers.begin(); i != reply->headers.end(); i++)
    {
      soup_message_headers_append(message->response_headers, i->first.c_str(), i->second.c_str());
    }
  
  soup_message_set_status(message, reply->status);
  soup_message_set_response(message, reply->content_type.c_str(), SOUP_MEMORY_COPY, reply->body.c_str(), reply->body.length());
}

int
HttpServer::get_port() const
{
  return port;
}

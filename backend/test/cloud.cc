// cloud.cc --- Main
//
// Copyright (C) 2012 Rob Caelers & Raymond Penners
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"
#include <fstream>
#include <stdio.h>

#include <glib-object.h>

#include "NetworkRouter.hh"

static gboolean on_timer(gpointer data)
{
  NetworkRouter *network = (NetworkRouter *)data;
  
  network->heartbeat();

  return G_SOURCE_CONTINUE;
}

int
main(int argc, char **argv)
{
  (void) argc;
  (void) argv;
  
#ifdef TRACING
  Debug::init();
#endif

  g_type_init();
  GMainLoop *loop= g_main_loop_new(NULL, FALSE);

  NetworkRouter network1("rob@workrave", "kjsdapkidszahf");
  network1.init(2701);

  NetworkRouter network2("rob@workrave", "kjsdapkidszahf");
  network2.init(2702);
  network2.connect("localhost", 2701);
  
  NetworkRouter network3("rob@workrave", "kjsdapkidszahf");
  network3.init(2703);
  network3.connect("localhost", 2701);

  NetworkRouter network4("rob@workrave", "kjsdapkidszahf");
  network4.init(2704);
  network4.connect("localhost", 2703);
  
  g_timeout_add_seconds(2, on_timer, &network1);
  g_main_loop_run(loop);
  
  return 0;
}

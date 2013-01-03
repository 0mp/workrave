// fog_test.cc
//
// Copyright (C) 2012, 2013 Rob Caelers <robc@krandor.nl>
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

#define BOOST_TEST_MODULE workrave
#include <boost/test/included/unit_test.hpp>

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>

#include "debug.hh"
#include "Workrave.hh"

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/signals2.hpp>

#include "utils/Timer.hh"


using namespace std;
using namespace workrave::utils;

struct Fixture
{
  Fixture()
  {
#ifdef TRACING
    Debug::init(boost::unit_test::framework::current_test_case().p_name.get() + "-");
    Debug::name(string("main"));
#endif

    XInitThreads();
    g_type_init();
    gtk_init(0, 0);

    BOOST_TEST_MESSAGE("constructing cores");

    const char *domains[] = { NULL, "Gtk", "GLib", "Gdk", "gtkmm", "GLib-GObject" };
    for (unsigned int i = 0; i < sizeof(domains)/sizeof(char *); i++)
      {
        g_log_set_handler(domains[i],
                          (GLogLevelFlags) (G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                          my_log_handler, NULL);
      }
    
    barrier = boost::shared_ptr<boost::barrier>(new boost::barrier(num_workraves + 1));

    for (int i = 0; i < num_workraves; i++)
      {
        workraves[i] = Workrave::create(i);
        workraves[i]->init(barrier);
      }

    barrier->wait();
    BOOST_TEST_MESSAGE("constructing cores...done");
    
    for (int i = 0; i < num_workraves; i++)
      {
        cores[i] = workraves[i]->get_core();

        fog[i] = workraves[i]->get_fog();
        fog_test[i] = boost::dynamic_pointer_cast<IFogTest>(fog[i]);
      }
  }
  
  ~Fixture()
  {
    BOOST_TEST_MESSAGE("destructing cores");
  
    for (int i = 0; i < num_workraves; i++)
      {
        workraves[i]->terminate();
      }
    sleep(5);
    
    //barrier->wait();
    BOOST_TEST_MESSAGE("destructing cores...done");
   }

  static void my_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                             const gchar *message, gpointer user_data)
  {
    TRACE_LOG("GLIB ERROR" << message);
    cerr << message << endl;
  }
  
protected:
  /*   /---\
   *   | 0 |
   *   \---/
   *     |
   *     |------|
   *     |      |
   *   /---\  /---\
   *   | 1 |  | 2 |
   *   \---/  \---/
   *     |      |
   *     |      |
   *   /---\  /---\
   *   | 3 |  | 4 |
   *   \---/  \---/
   *     |
   *     |
   *   /---\
   *   | 5 |
   *   \---/
   *     |
   *     |------|
   *     |      |
   *   /---\  /---\
   *   | 6 |  | 7 |
   *   \---/  \---/
   */

  void connect_all()
  {
    BOOST_TEST_MESSAGE("connecting cores...");
    for (int i = 0; i <= 6; i++)
      {
        connect_one(i);
      }
    sleep(5);
    BOOST_TEST_MESSAGE("connecting cores...done");
  }

  void connect_all_delayed()
  {
    BOOST_TEST_MESSAGE("connecting cores...");
    for (int i = 0; i <= 6; i++)
      {
        connect_one(i);
        g_usleep(G_USEC_PER_SEC);
      }
    sleep(5);
    BOOST_TEST_MESSAGE("connecting cores...done");
  }
  
  void connect_all_shuffled()
  {
    BOOST_TEST_MESSAGE("connecting cores...");
    int order[] = { 5,2,4,3,6,0,1 };
    
    for (int i = 0; i <= 6; i++)
      {
        connect_one(order[i]);
        g_usleep(G_USEC_PER_SEC);
      }
    sleep(5);
    BOOST_TEST_MESSAGE("connecting cores...done");
  }
  
  void connect_one(int c)
  {
    BOOST_TEST_MESSAGE("connect " + boost::lexical_cast<string>(c));
    switch(c)
      {
      case 0:
        workraves[1]->invoke(boost::bind(&Workrave::connect, workraves[1], "localhost", 2700));
        break;

      case 1:
        workraves[2]->invoke(boost::bind(&Workrave::connect, workraves[2], "localhost", 2700));
        break;

      case 2:
        workraves[3]->invoke(boost::bind(&Workrave::connect, workraves[3], "localhost", 2701));
        break;

      case 3:
        workraves[4]->invoke(boost::bind(&Workrave::connect, workraves[4], "localhost", 2702));
        break;

      case 4:
        workraves[5]->invoke(boost::bind(&Workrave::connect, workraves[5], "localhost", 2703));
        break;

      case 5:
        workraves[6]->invoke(boost::bind(&Workrave::connect, workraves[6], "localhost", 2705));
        break;
        
      case 6:
        workraves[7]->invoke(boost::bind(&Workrave::connect, workraves[7], "localhost", 2705));
        break;
      }
   
  } 

  void log(const string &txt)
  {
    for (int i = 0; i <  num_workraves; i++)
      {
        workraves[i]->invoke_sync(boost::bind(&Workrave::log, workraves[i], txt));
      }
  }


  void forall(int max, const std::string &msg, std::function<bool (int)> func)
  {
    int count = max;
    bool result = false;
    while (count > 0)
      {
        Timer::get()->simulate(1 * G_USEC_PER_SEC, G_USEC_PER_SEC / 2);

        result = false;
        for (int i = 0; i < num_workraves; i++)
          {
            result = workraves[i]->invoke_sync(boost::bind(func, i));
            if (!result)
              {
                break;
              }
          }

        if (result)
          {
            BOOST_TEST_MESSAGE(msg + " reached in " + boost::lexical_cast<string>(max-count+1));
            break;
          }
        count--;
      }

    BOOST_CHECK_MESSAGE(result, msg);
  }
  
  const static int num_workraves = 8;
  Workrave::Ptr workraves[num_workraves];
  ICore::Ptr cores[num_workraves];
  IFog::Ptr fog[num_workraves];
  IFogTest::Ptr fog_test[num_workraves];
  boost::shared_ptr<boost::barrier> barrier;
};

BOOST_FIXTURE_TEST_SUITE(s, Fixture)

BOOST_AUTO_TEST_CASE(test_propagate_operation_mode)
{
  connect_all();
  
  BOOST_TEST_MESSAGE("checking initial...");
  for (int i = 0; i <  num_workraves; i++)
    {
      OperationMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_operation_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, OPERATION_MODE_NORMAL);
    }
  
  BOOST_TEST_MESSAGE("set operation mode to suspended");
  workraves[0]->invoke_sync(boost::bind(&ICore::set_operation_mode, cores[0], OPERATION_MODE_SUSPENDED));
  sleep(2);

  for (int i = 0; i <  num_workraves; i++)
    {
      OperationMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_operation_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, OPERATION_MODE_SUSPENDED);
    }

  BOOST_TEST_MESSAGE("set operation mode to quiet");
  workraves[0]->invoke_sync(boost::bind(&ICore::set_operation_mode, cores[0], OPERATION_MODE_QUIET));
  sleep(2);

  for (int i = 0; i <  num_workraves; i++)
    {
      OperationMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_operation_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, OPERATION_MODE_QUIET);
    }

  BOOST_TEST_MESSAGE("set operation mode to normal");
  workraves[0]->invoke_sync(boost::bind(&ICore::set_operation_mode, cores[0], OPERATION_MODE_NORMAL));
  sleep(2);

  for (int i = 0; i <  num_workraves; i++)
    {
      OperationMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_operation_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, OPERATION_MODE_NORMAL);
    }
  BOOST_TEST_MESSAGE("done");
}


BOOST_AUTO_TEST_CASE(test_propagate_usage_mode)
{
  connect_all();
  
  for (int i = 0; i <  num_workraves; i++)
    {
      UsageMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_usage_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, USAGE_MODE_NORMAL);
    }
  
  workraves[0]->invoke_sync(boost::bind(&ICore::set_usage_mode, cores[0], USAGE_MODE_READING));
  sleep(2);

  for (int i = 0; i <  num_workraves; i++)
    {
      UsageMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_usage_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, USAGE_MODE_READING);
    }

  workraves[0]->invoke_sync(boost::bind(&ICore::set_usage_mode, cores[0], USAGE_MODE_NORMAL));
  sleep(2);

  for (int i = 0; i <  num_workraves; i++)
    {
      UsageMode mode = workraves[i]->invoke_sync(boost::bind(&ICore::get_usage_mode, cores[i]));
      BOOST_CHECK_EQUAL(mode, USAGE_MODE_NORMAL);
    }
}


BOOST_AUTO_TEST_CASE(test_connect_delayed)
{
  connect_all_delayed();

  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);
    }
}

BOOST_AUTO_TEST_CASE(test_connect_simultaneuously)
{
  connect_all();

  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);
    }
}

BOOST_AUTO_TEST_CASE(test_connect_shuffled)
{
  connect_all_shuffled();

  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);
    }
}

BOOST_AUTO_TEST_CASE(test_disconnect)
{
  connect_all();

  UUID id = workraves[1]->invoke_sync(boost::bind(&IFogTest::get_id, fog_test[1]));

  workraves[3]->invoke(boost::bind(&IFogTest::disconnect, fog_test[3], id));
  sleep(5);

  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 3);
    }
}


BOOST_AUTO_TEST_CASE(test_auto_connect)
{
  for (int i = 0; i < num_workraves; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);

  int total_direct = 0;
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);

      total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
    }
  BOOST_CHECK_EQUAL(total_direct, 14);
}

BOOST_AUTO_TEST_CASE(test_auto_connect_partial1)
{
  connect_one(0);
  connect_one(1);
  connect_one(2);
  sleep(5);
  
  for (int i = 0; i < num_workraves; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);

  int total_direct = 0;
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);

       total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
    }
  BOOST_CHECK_EQUAL(total_direct, 14);
}

BOOST_AUTO_TEST_CASE(test_auto_connect_partial2)
{
  connect_one(5);
  connect_one(6);
  connect_one(7);
  sleep(5);
  
  for (int i = 0; i < num_workraves; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);

  int total_direct = 0;
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);

      total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
    }
  BOOST_CHECK_EQUAL(total_direct, 14);
}


BOOST_AUTO_TEST_CASE(test_auto_connect_already_connected)
{
  connect_all();
  
  for (int i = 0; i < num_workraves; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);

  int total_direct = 0;
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);

      total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
    }
  BOOST_CHECK_EQUAL(total_direct, 14);
}


BOOST_AUTO_TEST_CASE(test_auto_connect_stages)
{
  for (int i = 0; i < 3; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), i < 3 ? 2 : 0);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);
    }
  
  for (int i = 3; i < 5; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), i < 5 ? 4 : 0);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);
    }

  for (int i = 4; i < 7; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(10);
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), i < 7 ? 6 : 0);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);
    }

  workraves[7]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[7]));
  sleep(10);
 
  int total_direct = 0;
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));      
      BOOST_CHECK_EQUAL(ids.size(), 7);

      int cycles = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_cycle_failures, fog_test[i]));
      BOOST_CHECK_EQUAL(cycles, 0);

      total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
    }
  BOOST_CHECK_EQUAL(total_direct, 14);
}


BOOST_AUTO_TEST_CASE(test_auto_reconnect)
{
  Timer::get()->set_simulated(true);

  connect_all();
  
  for (int i = 0; i < num_workraves; i++)
    {
      workraves[i]->invoke_sync(boost::bind(&IFogTest::start_announce, fog_test[i]));
    }

  sleep(1);
  Timer::get()->simulate(30 * G_USEC_PER_SEC);

  UUID id = workraves[1]->invoke_sync(boost::bind(&IFogTest::get_id, fog_test[3]));
  workraves[5]->invoke(boost::bind(&IFogTest::disconnect, fog_test[5], id));

  id = workraves[1]->invoke_sync(boost::bind(&IFogTest::get_id, fog_test[2]));
  workraves[4]->invoke(boost::bind(&IFogTest::disconnect, fog_test[4], id));

  sleep(1);
  Timer::get()->simulate(10 * G_USEC_PER_SEC);
  
  int total_direct = 0;
  for (int i = 0; i < num_workraves; i++)
    {
      list<UUID> ids = workraves[i]->invoke_sync(boost::bind(&IFogTest::get_clients, fog_test[i]));
      total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
    }
  BOOST_CHECK_EQUAL(total_direct, 10);

  const int max = 120;
  int count = max;
  while (count > 0)
    {
      Timer::get()->simulate(1 * G_USEC_PER_SEC);

      total_direct = 0;
      for (int i = 0; i < num_workraves; i++)
        {
          total_direct += workraves[i]->invoke_sync(boost::bind(&IFogTest::get_direct_clients, fog_test[i])).size();
        }
      if (total_direct == 14)
        {
          break;
        }
      count--;
    }
  BOOST_TEST_MESSAGE("reconnected in " << (max - count));
  BOOST_CHECK_PREDICATE(std::not_equal_to<int>(), (count)(0) ); 
}


BOOST_AUTO_TEST_CASE(test_activity)
{
  const gint64 delay = G_USEC_PER_SEC / 2;
  connect_all();

  forall(5, "Initially all", [&](int w) { return !cores[w]->is_user_active(); });

  Timer::get()->set_simulated(true);

  workraves[3]->set_active(true);
  g_usleep(delay);

  forall(5, "all active (1)", [&](int w) { return cores[w]->is_user_active(); });
  
  workraves[3]->set_active(false);
  g_usleep(delay);

  forall(5, "all idle (2)", [&](int w) { return !cores[w]->is_user_active(); });

  workraves[4]->set_active(true);
  g_usleep(delay);
  
  forall(5, "all active (3)", [&](int w) { return cores[w]->is_user_active(); });

  workraves[5]->set_active(true);
  g_usleep(delay);

  forall(5, "all active (4)", [&](int w) { return cores[w]->is_user_active(); });

  workraves[4]->set_active(false);
  g_usleep(delay);

  forall(5, "all active (5)", [&](int w) { return cores[w]->is_user_active(); });

  workraves[5]->set_active(false);
  g_usleep(delay);

  forall(5, "all idle (5)", [&](int w) { return !cores[w]->is_user_active(); });
}

BOOST_AUTO_TEST_SUITE_END()

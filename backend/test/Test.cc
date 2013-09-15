// Copyright (C) 2013 Rob Caelers
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
#include <boost/enable_shared_from_this.hpp>
#include <boost/signals2.hpp>

#ifdef HAVE_QT5
#include <QtCore>
#endif

#include "ICore.hh"
#include "IApp.hh"
#include "IBreak.hh"

#include "utils/ITimeSource.hh"
#include "utils/TimeSource.hh"
#include "config/Config.hh"
#include "ICoreTestHooks.hh"

#include "Test.hh"

using namespace std;
using namespace workrave::utils;
using namespace workrave::config;

class SimulatedTime : public ITimeSource, public boost::enable_shared_from_this<SimulatedTime>
{
public:
  typedef boost::shared_ptr<SimulatedTime> Ptr;
  
  void init()
  {
    current_time = TimeSource::get_monotonic_time_usec();
    TimeSource::source = shared_from_this();
  }
  
  virtual int64_t get_real_time_usec()
  {
    return current_time;
  }
  
  virtual int64_t get_monotonic_time_usec()
  {
    return current_time;
  }

  int64_t current_time;
};

class Backend : public workrave::IApp
{
public:
  Backend()
  {
    string display_name = "";

#ifdef HAVE_QT5
    int argc = 1;
    char argv1[] = "test";
    char* argv[] = {argv1, NULL};
    app = new QCoreApplication(argc, argv);
#endif

    core = workrave::ICore::create();
    
    ICoreHooks::Ptr hooks = core->get_hooks();
    ICoreTestHooks::Ptr test_hooks = boost::dynamic_pointer_cast<ICoreTestHooks>(hooks);
    
    //test_hooks->hook_local_activity_state() = boost::bind(&Workrave::on_local_activity_state, this);
    test_hooks->hook_create_configurator() = boost::bind(&Backend::on_create_configurator, this);
    
    core->init(this, "");

    for (int i = 0; i < workrave::BREAK_ID_SIZEOF; i++)
      {
        workrave::IBreak::Ptr b = core->get_break(workrave::BreakId(i));
        b->signal_break_event().connect(boost::bind(&Backend::on_break_event, this, workrave::BreakId(i), _1));
      }

    core->signal_operation_mode_changed().connect(boost::bind(&Backend::on_operation_mode_changed, this, _1)); 

    sim = SimulatedTime::Ptr(new SimulatedTime());
  }
  
  ~Backend()
  {
#ifdef HAVE_QT5
    delete app;
#endif
    BOOST_TEST_MESSAGE("destructing...done");
   }

  virtual void create_prelude_window(workrave::BreakId break_id)
  {
  }
  
  virtual void create_break_window(workrave::BreakId break_id, workrave::BreakHint break_hint)
  {
  }
  
  virtual void hide_break_window()
  {
  }
  
  virtual void show_break_window()
  {
  }
  
  virtual void refresh_break_window()
  {
  }
  
  virtual void set_break_progress(int value, int max_value)
  {
  }
  
  virtual void set_prelude_stage(PreludeStage stage)
  {
  }
  
  virtual void set_prelude_progress_text(PreludeProgressText text)
  {
  }

  virtual void on_break_event(workrave::BreakId break_id, workrave::BreakEvent event)
  {
  }
  
  virtual void on_operation_mode_changed(const workrave::OperationMode m)
  {
  }

  IConfigurator::Ptr on_create_configurator()
  {
    IConfigurator::Ptr configurator = ConfiguratorFactory::create(ConfiguratorFactory::FormatIni);
    
    configurator->set_value("plugins/networking/user", "robc@workrave");
    configurator->set_value("plugins/networking/secret", "HelloWorld");
    
    return configurator;
  }
  
  workrave::ICore::Ptr core;
  SimulatedTime::Ptr sim;
#ifdef HAVE_QT5
  QCoreApplication *app;
#endif
};

BOOST_FIXTURE_TEST_SUITE(s, Backend)

BOOST_AUTO_TEST_CASE(test_limit_reached)
{
  sim->init();
}

BOOST_AUTO_TEST_SUITE_END()

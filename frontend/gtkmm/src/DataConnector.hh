// DataConnector.hh --- Connect widget with the configurator
//
// Copyright (C) 2007 Rob Caelers <robc@krandor.nl>
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
// $Id: ITimeBar.hh 1090 2006-10-01 20:49:47Z dotsphinx $
//

#ifndef DATACONNECTOR_HH
#define DATACONNECTOR_HH

#include <string>
#include <list>
#include <sigc++/sigc++.h>

#include "IConfiguratorListener.hh"
#include "ICore.hh"

namespace workrave
{
  class IConfigurator;
}

class TimeEntry;

namespace Gtk
{
  class Object;
  class Widget;
  class Entry;
  class SpinButton;
  class OptionMenu;
  class Adjustment;
}

class DataConnection;

namespace dc
{
  enum Flags
    {
      NONE = 0,
      NO_CONFIG = 1,
      NO_WIDGET = 2,
    };
}


class DataConnector
{
public:
  DataConnector();
  ~DataConnector();

  void connect(workrave::BreakId id,
               const std::string &setting,
               DataConnection *connection,
               dc::Flags flags = dc::NONE);

  void connect(const std::string &setting,
               DataConnection *connection,
               dc::Flags flags = dc::NONE);

  void intercept_last(sigc::slot<bool, const std::string &, bool> slot);

private:
  struct MonitoredWidget
  {
    MonitoredWidget()
    {
      connection = NULL;
    }

    DataConnection *connection;
  };

  typedef std::list<MonitoredWidget> Widgets;
  typedef Widgets::iterator WidgetIter;

  //! 
  Widgets connections;

  //!
  workrave::IConfigurator *config;

  //! Last
  DataConnection *last_connection;
};



class DataConnection
  : public workrave::IConfiguratorListener
{
public:
  DataConnection();
  virtual ~DataConnection();

  void set(dc::Flags flags, const std::string &key);
  virtual void init() = 0;

  sigc::signal<bool, const std::string &, bool> intercept;

protected:

  workrave::IConfigurator *config;
  std::string key;
  dc::Flags flags;
};


#define DECLARE_DATA_TYPE(WidgetType, WrapperType, WidgetDataType/*, ConfigDataType */) \
  class WrapperType : public DataConnection                             \
  {                                                                     \
  public:                                                               \
    WrapperType(WidgetType *widget)                                     \
      : widget(widget)                                                  \
      {                                                                 \
      }                                                                 \
    virtual ~WrapperType()                                              \
    {                                                                   \
    }                                                                   \
                                                                        \
    void init();                                                        \
    void widget_changed_notify();                                       \
    void config_changed_notify(const std::string &key);                 \
                                                                        \
  private:                                                              \
    WidgetType *widget;                                                 \
  };                                                                    \
                                                                        \
  namespace dc {                                                        \
    WrapperType *wrap (WidgetType *t);                                  \
  }

#define DEFINE_DATA_TYPE(WidgetType, WrapperType)                       \
  namespace dc {                                                        \
    WrapperType *wrap (WidgetType *t)                                   \
    {                                                                   \
      if (t != NULL)                                                    \
        {                                                               \
          return new WrapperType(t);                                    \
        }                                                               \
      else                                                              \
        {                                                               \
          return NULL;                                                  \
        }                                                               \
    }                                                                   \
  }


DECLARE_DATA_TYPE(Gtk::Entry, DataConnectionGtkEntry, std::string);
DECLARE_DATA_TYPE(Gtk::CheckButton, DataConnectionGtkCheckButton, bool);
DECLARE_DATA_TYPE(Gtk::SpinButton, DataConnectionGtkSpinButton, int);
DECLARE_DATA_TYPE(Gtk::OptionMenu, DataConnectionGtkOptionMenu, int);
DECLARE_DATA_TYPE(Gtk::Adjustment, DataConnectionGtkAdjustment, int);
DECLARE_DATA_TYPE(TimeEntry, DataConnectionTimeEntry, int);


class DataConnectionGtkEntryTwin  : public DataConnection
{
  public:
  DataConnectionGtkEntryTwin(Gtk::Entry *widget1, Gtk::Entry *widget2)
    : widget1(widget1), widget2(widget2)
  {
  }
  virtual ~DataConnectionGtkEntryTwin()
  {
  }

  void init();
  void widget_changed_notify();
  void config_changed_notify(const std::string &key);

private:
  Gtk::Entry *widget1;
  Gtk::Entry *widget2;
};

namespace dc
{
  DataConnectionGtkEntryTwin *wrap(Gtk::Entry *w1, Gtk::Entry *w2);
}

#endif

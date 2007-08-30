// Configurator.hh
//
// Copyright (C) 2001, 2002, 2003, 2006, 2007 Rob Caelers <robc@krandor.org>
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

#ifndef CONFIGURATOR_HH
#define CONFIGURATOR_HH

#include <string>
#include <list>
#include <map>

#include "Mutex.hh"

using namespace std;

#include "IConfigurator.hh"

class ConfiguratorListener;

class Configurator : public IConfigurator
{
public:
  typedef enum Format
    {
      FormatIni,
      FormatXml,
      FormatNative
    };
  virtual ~Configurator();

  //! Creates a Configurator with the specified type.
  static Configurator *create(Format);

  //! Loads the specified file.
  /*!
   *  \param filename file to load.
   *
   *  \retval true load succeeded.
   *  \retval false load failed.
   */
  virtual bool load(string filename) = 0;

  //! Saves the configuration to the specified file.
  /*!
   *  \param filename file to save.
   *
   *  \retval true save succeeded.
   *  \retval false save failed.
   */
  virtual bool save(string filename) = 0;

  //! Saves the configuration.
  /*!
   *  \retval true save succeeded.
   *  \retval false save failed.
   */
  virtual bool save() = 0;

  //! Returns the value of the specified attribute
  /*!
   *  \retval true value successfully returned.
   *  \retval false attribute not found.
   */
  virtual bool get_value(string key, string *out) const = 0;
  void get_value_default(string key, string *out, string s) const;
  bool get_value_on_quit( string key, string *out ) const;

  //! Returns the value of the specified attribute
  /*!
   *  \retval true value successfully returned.
   *  \retval false attribute not found.
   */
  virtual bool get_value(string key, bool *out) const = 0;
  void get_value_default(string key, bool *out, const bool def) const;
  bool get_value_on_quit( string key, bool *out ) const;

  //! Returns the value of the specified attribute
  /*!
   *  \retval true value successfully returned.
   *  \retval false attribute not found.
   */
  virtual bool get_value(string key, int *out) const = 0;
  void get_value_default(string key, int *out, const int def) const;
  bool get_value_on_quit( string key, int *out ) const;

  //! Returns the value of the specified attribute
  /*!
   *  \retval true value successfully returned.
   *  \retval false attribute not found.
   */
  virtual bool get_value(string key, long *out) const = 0;
  void get_value_default(string key, long *out, const long def) const;
  bool get_value_on_quit( string key, long *out ) const;

  //! Returns the value of the specified attribute
  /*!
   *  \retval true value successfully returned.
   *  \retval false attribute not found.
   */
  virtual bool get_value(string key, double *out) const = 0;
  void get_value_default(string key, double *out, const double def) const;
  bool get_value_on_quit( string key, double *out ) const;

  //! Sets the value of the specified attribute.
  /*!
   *  \retval true attribute successfully set.
   *  \retval false attribute could not be set..
   */
  virtual bool set_value(string key, string v) = 0;
  void set_value_on_quit( string key, string v );

  //! Sets the value of the specified attribute.
  /*!
   *  \retval true attribute successfully set.
   *  \retval false attribute could not be set..
   */
  virtual bool set_value(string key, int v) = 0;
  void set_value_on_quit( string key, int v );

  //! Sets the value of the specified attribute.
  /*!
   *  \retval true attribute successfully set.
   *  \retval false attribute could not be set..
   */
  virtual bool set_value(string key, long v) = 0;
  void set_value_on_quit( string key, long v );

  //! Sets the value of the specified attribute.
  /*!
   *  \retval true attribute successfully set.
   *  \retval false attribute could not be set..
   */
  virtual bool set_value(string key, bool v) = 0;
  void set_value_on_quit( string key, bool v );

  //! Sets the value of the specified attribute.
  /*!
   *  \retval true attribute successfully set.
   *  \retval false attribute could not be set..
   */
  virtual bool set_value(string key, double v) = 0;
  void set_value_on_quit( string key, double v );

  //! Adds the specified configuration change listener.
  /*!
   *  \param key_prefix configuration path to monitor.
   *  \param listener listener to add.
   *
   *  \retval true listener successfully added.
   *  \retval false listener already added.
   */
  virtual bool add_listener(string key_prefix, ConfiguratorListener *listener);

  //! Removes the specified configuration change listener.
  /*!
   *  \param listener listener to stop monitoring.
   *
   *  \retval true listener successfully removed.
   *  \retval false listener not found.
   */
  virtual bool remove_listener(ConfiguratorListener *listener);

  //! Removes the specified configuration change listener.
  /*!
   *  \param key_prefix configuration path to monitor.
   *  \param listener listener to stop monitoring.
   *
   *  \retval true listener successfully removed.
   *  \retval false listener not found.
   */
  virtual bool remove_listener(string key_prefix, ConfiguratorListener *listener);

  //! Finds the key monitored by the the specified configuration change listener.
  /*!
   *  \param listener listener to find the key of.
   *
   */
  virtual bool find_listener(ConfiguratorListener *listener, string &key) const;

  //! Sets keys/values that are in the set_value_on_quit keylist.
  //! Call from the Core destructor before deleting configurator.
  void set_values_now_we_are_quitting();


protected:
  void fire_configurator_event(string key);
  void strip_leading_slash(string &key) const;
  void strip_trailing_slash(string &key) const;
  void add_trailing_slash(string &key) const;

  Configurator();

protected:
  typedef std::list<std::pair<string, ConfiguratorListener *> > Listeners;
  typedef std::list<std::pair<string, ConfiguratorListener *> >::iterator ListenerIter;
  typedef std::list<std::pair<string, ConfiguratorListener *> >::const_iterator ListenerCIter;

  //! Configuration change listeners.
  Listeners listeners;


private:

  //! Internal locking
  Mutex *mutex;

  //! struct of a list member
  //! list will contain keys/values to set on quit
  typedef struct keylist
    {
      string key;
      string v;
      keylist *next;
      keylist()
        {
          key = "";
          v = "";
          next = NULL;
        }
    };

  //! pointer to the head of the list
  keylist *head;

};

#endif // CONFIGURATOR_HH

// GUIControl.hh --- The WorkRave GUI
//
// Copyright (C) 2001, 2002 Rob Caelers & Raymond Penners
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

#ifndef GUICONTROL_HH
#define GUICONTROL_HH

#include "ConfiguratorListener.hh"

class GUIFactoryInterface;
class BreakWindowInterface;
class PreludeWindowInterface;

class Configurator;
class BreakControl;
class ControlInterface;
class BreakInterface;
class TimerInterface;
class SoundPlayerInterface;

class GUIControl :
  public ConfiguratorListener
{
public:
  //! Constructor.
  GUIControl(GUIFactoryInterface *factory, ControlInterface *controller);

  //! Destructor
  virtual ~GUIControl();

  enum OperationMode
    {
      OPERATION_MODE_NORMAL,
      OPERATION_MODE_SUSPENDED,
      OPERATION_MODE_QUIET,
      OPERATION_MODE_SIZEOF
    };
  
  enum BreakId
    {
      BREAK_ID_NONE = -1,
      BREAK_ID_MICRO_PAUSE = 0,
      BREAK_ID_REST_BREAK,
      BREAK_ID_DAILY_LIMIT,
      BREAK_ID_SIZEOF
    };

  enum TimerId
    {
      TIMER_ID_NONE = -1,
      TIMER_ID_MICRO_PAUSE = 0,
      TIMER_ID_REST_BREAK,
      TIMER_ID_DAILY_LIMIT,
      TIMER_ID_SIZEOF
    };


  enum BreakAction
    {
      BREAK_ACTION_NONE,
      BREAK_ACTION_START_BREAK,
      BREAK_ACTION_STOP_BREAK,
      BREAK_ACTION_NATURAL_STOP_BREAK,
      BREAK_ACTION_FORCE_START_BREAK,
    };

  struct TimerData
  {
    TimerInterface *timer;
    BreakControl *break_control;
    const char *label;
    string icon;
    int break_id;
    string break_name;
    
    TimerData();
    
    int get_break_max_preludes() const;
    bool get_break_force_after_preludes() const;
    bool get_break_ignorable() const;
    bool get_break_insisting() const;

    void set_break_max_preludes(int n);
    void set_break_force_after_preludes(bool b);
    void set_break_ignorable(bool b);
    void set_break_insisting(bool b);
  };
  TimerData timers[BREAK_ID_SIZEOF];
  
public:
  static GUIControl *get_instance();
  SoundPlayerInterface *get_sound_player();
  void heartbeat();
  void init();
  OperationMode set_operation_mode(OperationMode mode);
  void break_action(BreakId id, BreakAction action);
  Configurator *get_configurator();
  void set_freeze_all_breaks(bool freeze);
  
private:
  void stop_all_breaks();
  void restart_break();
  
  void update_statistics();
  void handle_start_break(BreakInterface *breaker, BreakId break_id, TimerInterface *timer);
  void handle_stop_break(BreakInterface *breaker, BreakId break_id, TimerInterface *timer);
  bool load_config();
  bool store_config();
  bool verify_config();
  void load_break_control_config(string break_id);
  void load_break_control_config(int break_id);
  void config_changed_notify(string key);
  
public:
  static const string CFG_KEY_BREAKS;
  static const string CFG_KEY_BREAK;
  static const string CFG_KEY_BREAK_MAX_PRELUDES;
  static const string CFG_KEY_BREAK_FORCE_AFTER_PRELUDES;
  static const string CFG_KEY_BREAK_IGNORABLE;
  static const string CFG_KEY_BREAK_INSISTING;
  static const string CFG_KEY_MAIN_WINDOW;
  static const string CFG_KEY_MAIN_WINDOW_ALWAYS_ON_TOP;
  
private:
  //! The one and only instance
  static GUIControl *instance;

  //! GUI Widget factory.
  GUIFactoryInterface *gui_factory;
  
  //! The Controller
  ControlInterface *core_control;

  //! The sound player
  SoundPlayerInterface *sound_player;
  
  //! Configuration access.
  Configurator *configurator;

  //! Mode.
  OperationMode operation_mode;
};


inline GUIControl *
GUIControl::get_instance()
{
  return instance;
}

inline SoundPlayerInterface *
GUIControl::get_sound_player()
{
  return sound_player;
}

#endif // GUICONTROL_HH

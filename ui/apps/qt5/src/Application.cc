// Copyright (C) 2001 - 2013 Rob Caelers & Raymond Penners
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

#include "Application.hh"

#include "commonui/nls.h"
#include "debug.hh"

#include "commonui/Backend.hh"
#include "commonui/GUIConfig.hh"
#include "config/IConfigurator.hh"
#include "core/IBreak.hh"
#include "core/ICore.hh"
#include "session/System.hh"
#include "utils/Exception.hh"
#include "utils/Platform.hh"

// DBus
#if defined(interface)
#undef interface
#endif
#include "dbus/IDBus.hh"

using namespace std;
using namespace workrave;
using namespace workrave::utils;

Application::Application(int argc, char **argv, std::shared_ptr<IToolkit> toolkit) :
  toolkit(toolkit),
  active_break_id(BREAK_ID_NONE),
  muted(false)
  //closewarn_shown(false)
{
  TRACE_ENTER("GUI:GUI");

  this->argc = argc;
  this->argv = argv;

  TRACE_EXIT();
}


Application::~Application()
{
  TRACE_ENTER("GUI:~GUI");

  core.reset();

  TRACE_EXIT();
}


void
Application::restbreak_now()
{
  core->force_break(BREAK_ID_REST_BREAK, BREAK_HINT_USER_INITIATED);
}

void
Application::main()
{
  TRACE_ENTER("Application::main");

  init_core();

  menus = std::make_shared<Menus>(shared_from_this(), toolkit, core);

  init_sound_player();

  toolkit->init(menus->get_menu_model(), sound_theme);

  init_nls();
  init_platform();
  init_bus();
  init_session();
  init_startup_warnings();
  init_updater();

  connections.connect(toolkit->signal_timer(), std::bind(&Application::on_timer, this));
  on_timer();

  TRACE_MSG("Initialized. Entering event loop.");
  toolkit->run();

  TRACE_EXIT();
}

void
Application::terminate()
{
  TRACE_ENTER("Application::terminate");

  Backend::get_configurator()->save();

  toolkit->terminate();

  TRACE_EXIT();
}

bool
Application::on_timer()
{
  core->heartbeat();

  if (!break_windows.empty() && muted)
    {
      bool user_active = core->is_user_active();

      if (user_active)
        {
          sound_theme->restore_mute();
          muted = false;
        }
    }

 return true;
}

void
Application::init_platform()
{
 System::init();
 srand((unsigned int)time(NULL));
}

void
Application::init_session()
{
 session = std::make_shared<Session>();
 session->init();
}

void
Application::init_nls()
{
//#if defined(ENABLE_NLS)
//  string language = GUIConfig::locale();
//  if (language != "")
//    {
//      g_setenv("LANGUAGE", language.c_str(), 1);
//    }
//
//#  if !defined(HAVE_GTK3)
//  gtk_set_locale();
//#  endif
//  const char *locale_dir;
//
//#if defined(PLATFORM_OS_WIN32)
//  string dir = Util::get_application_directory();
//  // Use the pre-install locale location if workrave is running from its MSVC build directory.
//  dir += Util::file_exists( dir + "\\..\\Workrave.sln" ) ? "\\..\\ui" : "\\lib\\locale";
//  locale_dir = dir.c_str();
//#elif defined(PLATFORM_OS_OSX)
//  char locale_path[MAXPATHLEN * 4];
//  char execpath[MAXPATHLEN+1];
//  uint32_t pathsz = sizeof (execpath);
//
//  _NSGetExecutablePath(execpath, &pathsz);
//
//  gchar *dir_path = g_path_get_dirname(execpath);
//  strcpy(locale_path, dir_path);
//  g_free(dir_path);
//
//  // Locale
//  strcat(locale_path, "/../Resources/locale");
//  locale_dir = locale_path;
//#else
//  locale_dir = GNOMELOCALEDIR;
//#  endif
//
//#  ifdef HAVE_SETLOCALE
//  setlocale(LC_ALL, "");
//#  endif
//
//#if defined(PLATFORM_OS_WIN32)
//  bindtextdomain("gtk20", locale_dir);
//  bindtextdomain("iso_3166", locale_dir);
//  bindtextdomain("iso_639", locale_dir);
//  bindtextdomain("glib20", locale_dir);
//  bind_textdomain_codeset("gk20", "UTF-8");
//  bind_textdomain_codeset("glib20", "UTF-8");
//  bind_textdomain_codeset("iso_3166", "UTF-8");
//  bind_textdomain_codeset("iso_639", "UTF-8");
//
//  Backend::get_configurator()->add_listener(GUIConfig::CFG_KEY_LOCALE, this);
//#endif
//
//  bindtextdomain(GETTEXT_PACKAGE, locale_dir);
//  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
//  textdomain(GETTEXT_PACKAGE);
//
//#endif
}


//! Initializes the core.
void
Application::init_core()
{
  core = Backend::get_core();
  core->init(this, toolkit->get_display_name());

  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      IBreak::Ptr b = core->get_break(BreakId(i));
      connections.connect(b->signal_break_event(), std::bind(&Application::on_break_event, this, BreakId(i), std::placeholders::_1));
    }

  GUIConfig::init();
}


void
Application::init_bus()
{
 workrave::dbus::IDBus::Ptr dbus = Backend::get_dbus();

 if (dbus->is_available())
   {
     if (dbus->is_running("org.workrave.Workrave"))
       {
         // TODO:
         
         // Gtk::MessageDialog dialog(tr("Workrave failed to start"),
         //                           false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
         // dialog.set_secondary_text(tr("Is Workrave already running?"));
         // dialog.show();
         // dialog.run();
         exit(1);
       }

#ifdef HAVE_DBUS
     try
       {
         dbus->register_object_path("/org/workrave/Workrave/UI");
         dbus->register_service("org.workrave.Workrave");

         extern void init_DBusGUI(workrave::dbus::IDBus::Ptr dbus);
         init_DBusGUI(dbus);

         dbus->connect("/org/workrave/Workrave/UI",
                      "org.workrave.ControlInterface",
                       menus.get());
       }
     catch (workrave::dbus::DBusException &)
       {
       }
#endif
   }
}

void
Application::init_startup_warnings()
{
 OperationMode mode = core->get_operation_mode();
 if (mode != OperationMode::Normal)
   {
     toolkit->create_oneshot_timer(5000, std::bind(&Application::on_operation_mode_warning_timer, this));
   }
}

void
Application::init_updater()
{
  // TODO:
  // updater = workrave::updater::std::make_shared<Updater>("http://snapshots.workrave.org/appcast/");
  // if (updater)
  //   {
  //     updater->check_for_updates();
  //   }
}

void
Application::init_sound_player()
{
  try
    {
      // Tell pulseaudio were are playing sound events
      Platform::setenv("PULSE_PROP_media.role", "event", 1);

      sound_theme = std::make_shared<SoundTheme>();
      sound_theme->init();
    }
  catch (workrave::utils::Exception)
    {
    }
}


void
Application::on_break_event(BreakId break_id, BreakEvent event)
{
  TRACE_ENTER_MSG("Application::on_break_event", break_id << " "
                  << static_cast<std::underlying_type<BreakEvent>::type>(event));

  struct EventMap
  {
    BreakId id;
    BreakEvent break_event;
    SoundEvent sound_event;
  } event_mappings[] =
      {
        { BREAK_ID_MICRO_BREAK, BreakEvent::ShowPrelude,    SoundEvent::BreakPrelude },
        { BREAK_ID_MICRO_BREAK, BreakEvent::BreakIgnored,   SoundEvent::BreakIgnored },
        { BREAK_ID_MICRO_BREAK, BreakEvent::ShowBreak,      SoundEvent::MicroBreakStarted },
        { BREAK_ID_MICRO_BREAK, BreakEvent::BreakTaken,     SoundEvent::MicroBreakEnded },
        { BREAK_ID_REST_BREAK,  BreakEvent::ShowPrelude,    SoundEvent::BreakPrelude },
        { BREAK_ID_REST_BREAK,  BreakEvent::BreakIgnored,   SoundEvent::BreakIgnored },
        { BREAK_ID_REST_BREAK,  BreakEvent::ShowBreak,      SoundEvent::RestBreakStarted },
        { BREAK_ID_REST_BREAK,  BreakEvent::BreakTaken,     SoundEvent::RestBreakEnded },
        { BREAK_ID_DAILY_LIMIT, BreakEvent::ShowPrelude,    SoundEvent::BreakPrelude},
        { BREAK_ID_DAILY_LIMIT, BreakEvent::BreakIgnored,   SoundEvent::BreakIgnored},
        { BREAK_ID_DAILY_LIMIT, BreakEvent::ShowBreak,      SoundEvent::MicroBreakEnded },
      };

  for (auto &event_mapping : event_mappings)
    {
      if (event_mapping.id == break_id && event_mapping.break_event == event)
        {
          bool mute = false;
          SoundEvent snd = event_mapping.sound_event;
          TRACE_MSG("play " << static_cast<std::underlying_type<BreakEvent>::type>(event));

          mute = SoundTheme::sound_mute()();
          if (mute)
            {
              muted = true;
            }
          TRACE_MSG("Mute after playback " << mute);
          sound_theme->play_sound(snd, mute);
        }
    }
}

//   if (event == CORE_EVENT_MONITOR_FAILURE)
//     {
//       string msg = tr("Workrave could not monitor your keyboard and mouse activity.\n");

// #ifdef PLATFORM_OS_UNIX
//       msg += tr("Make sure that the RECORD extension is enabled in the X server.");
// #endif
//       Gtk::MessageDialog dialog(tr("Workrave failed to start"),
//                                 false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
//       dialog.set_secondary_text(msg);
//       dialog.show();
//       dialog.run();
//       terminate();
//     }
//   TRACE_EXIT();
// }


// TODO: locale update

// void
// Application::config_changed_notify(const std::string &key)
// {
//   TRACE_ENTER_MSG("Application::config_changed_notify", key);

// #if defined(HAVE_LANGUAGE_SELECTION)
//   if (key == GUIConfig::locale().key())
//     {
//       string locale = GUIConfig::locale();
//       Locale::set_locale(locale);

//       // menus->locale_changed();
//     }
// #endif

//   TRACE_EXIT();
// }

void
Application::create_prelude_window(BreakId break_id)
{
  hide_break_window();
  active_break_id = break_id;

  for (int i = 0; i < toolkit->get_screen_count(); i++)
    {
      prelude_windows.push_back(toolkit->create_prelude_window(i, break_id));
    }
}

void
Application::create_break_window(BreakId break_id, BreakHint break_hint)
{
  TRACE_ENTER("Application::start_break_window");
  hide_break_window();

  BreakFlags break_flags = BREAK_FLAGS_NONE;
  bool ignorable = GUIConfig::break_ignorable(break_id)();
  bool skippable = GUIConfig::break_skippable(break_id)();

  if (break_hint & BREAK_HINT_USER_INITIATED)
  {
      break_flags = ( BREAK_FLAGS_POSTPONABLE |
                      BREAK_FLAGS_USER_INITIATED);

      if (skippable)
        {
          break_flags |=  BREAK_FLAGS_SKIPPABLE;
        }
    }
  else
    {
      if (ignorable)
        {
          break_flags |= BREAK_FLAGS_POSTPONABLE;
        }

      if(skippable)
        {
          break_flags |= BREAK_FLAGS_SKIPPABLE;
        }
    }

  if (break_hint & BREAK_HINT_NATURAL_BREAK)
    {
      break_flags |=  (BREAK_FLAGS_NO_EXERCISES | BREAK_FLAGS_NATURAL |
                       BREAK_FLAGS_POSTPONABLE);
    }

  active_break_id = break_id;

  for (int i = 0; i < toolkit->get_screen_count(); i++)
    {
      IBreakWindow::Ptr break_window = toolkit->create_break_window(i, break_id, break_flags);
      break_window->init();
      break_windows.push_back(break_window);
    }

  TRACE_EXIT();
}

void
Application::hide_break_window()
{
  TRACE_ENTER("Application::hide_break_window");
  active_break_id = BREAK_ID_NONE;

  for (auto &window : prelude_windows)
    {
      window->stop();
    }

  for (auto &window : break_windows)
    {
      window->stop();
    }

  break_windows.clear();
  prelude_windows.clear();

  TRACE_EXIT();
}

void
Application::show_break_window()
{
  TRACE_ENTER("Application::hide_break_window");

  for (auto &window : prelude_windows)
    {
      window->start();
    }

  for (auto &window : break_windows)
    {
      window->start();
    }

  TRACE_EXIT();
}

void
Application::refresh_break_window()
{
  for (auto &window : prelude_windows)
    {
      window->refresh();
    }

  for (auto &window : break_windows)
    {
      window->refresh();
    }
}

void
Application::set_break_progress(int value, int max_value)
{
  for (auto &window : prelude_windows)
    {
      window->set_progress(value, max_value);
    }

  for (auto &window : break_windows)
    {
      window->set_progress(value, max_value);
    }
}

void
Application::set_prelude_stage(PreludeStage stage)
{
  for (auto &window : prelude_windows)
    {
      window->set_stage(stage);
    }
}

void
Application::set_prelude_progress_text(PreludeProgressText text)
{
  for (auto &window : prelude_windows)
    {
      window->set_progress_text(text);
    }
}

void
Application::on_operation_mode_warning_timer()
{
  OperationMode mode = core->get_operation_mode();
  if (mode == OperationMode::Suspended)
    {
      toolkit->show_balloon("operation_mode",
                            tr("Workrave").toStdString(),
                            tr("Workrave is in suspended mode. "
                               "Mouse and keyboard activity will not be monitored.").toStdString());
    }
  else if (mode == OperationMode::Quiet)
    {
      toolkit->show_balloon("operation_mode",
                            tr("Workrave").toStdString(),
                            tr("Workrave is in quiet mode. "
                               "No break windows will appear.").toStdString());
    }
}

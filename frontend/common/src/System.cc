// System.cc
//
// Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2011 Rob Caelers & Raymond Penners
// Copyright (C) 2014 Mateusz Jończyk
// All rights reserved.
// Some lock commands are imported from the KShutdown utility:
//          http://kshutdown.sourceforge.net/
//          file src/actions/lock.cpp
//          Copyright (C) 2009  Konrad Twardowski
// Mateusz Jończyk has read source code of xflock4, lxlock and enlightenment_remote,
// but that did not influence the code in any way except for getting simple info on how
// they work.
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

#ifdef HAVE_GLIB
#include <glib.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <iostream>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "System.hh"
#include "debug.hh"

#if defined(PLATFORM_OS_UNIX)
#include "ScreenLockCommandline.hh"

#ifdef HAVE_DBUS_GIO
#include "ScreenLockDBus.hh"
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef HAVE_APP_GTK
#include <gdk/gdkx.h>
#endif
#endif //PLATFORM_OS_UNIX

#if defined(HAVE_UNIX)
#include <sys/wait.h>
#endif

#ifdef PLATFORM_OS_WIN32
#include <shlobj.h>
#include <shldisp.h>
#include "harpoon.h"

#include "CoreFactory.hh"
#include "IConfigurator.hh"

using namespace workrave;

#ifndef HAVE_ISHELLDISPATCH
#undef INTERFACE
#define INTERFACE IShellDispatch
DECLARE_INTERFACE_(IShellDispatch, IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD_(ULONG,dummy1)(THIS) PURE;
  STDMETHOD_(ULONG,dummy2)(THIS) PURE;
  STDMETHOD_(ULONG,dummy3)(THIS) PURE;
  STDMETHOD_(ULONG,dummy4)(THIS) PURE;
  STDMETHOD_(ULONG,dummy5)(THIS) PURE;
  STDMETHOD_(ULONG,dummy6)(THIS) PURE;
  STDMETHOD_(ULONG,dummy7)(THIS) PURE;
  STDMETHOD_(ULONG,dummy8)(THIS) PURE;
  STDMETHOD_(ULONG,dummy9)(THIS) PURE;
  STDMETHOD_(ULONG,dummya)(THIS) PURE;
  STDMETHOD_(ULONG,dummyb)(THIS) PURE;
  STDMETHOD_(ULONG,dummyc)(THIS) PURE;
  STDMETHOD_(ULONG,dummyd)(THIS) PURE;
  STDMETHOD_(ULONG,dummye)(THIS) PURE;
  STDMETHOD_(ULONG,dummyf)(THIS) PURE;
  STDMETHOD_(ULONG,dummyg)(THIS) PURE;
  STDMETHOD_(ULONG,dummyh)(THIS) PURE;
        STDMETHOD(ShutdownWindows)(THIS) PURE;
  STDMETHOD_(ULONG,dummyi)(THIS) PURE;
  STDMETHOD_(ULONG,dummyj)(THIS) PURE;
  STDMETHOD_(ULONG,dummyk)(THIS) PURE;
  STDMETHOD_(ULONG,dummyl)(THIS) PURE;
  STDMETHOD_(ULONG,dummym)(THIS) PURE;
  STDMETHOD_(ULONG,dummyn)(THIS) PURE;
  STDMETHOD_(ULONG,dummyo)(THIS) PURE;
  STDMETHOD_(ULONG,dummyp)(THIS) PURE;
  STDMETHOD_(ULONG,dummyq)(THIS) PURE;
END_INTERFACE
};
typedef IShellDispatch *LPSHELLDISPATCH;
#endif

//uuid(D8F015C0-C278-11CE-A49E-444553540000);
const GUID IID_IShellDispatch =
{
  0xD8F015C0, 0xc278, 0x11ce,
  { 0xa4, 0x9e, 0x44, 0x45, 0x53, 0x54 }
};
// 13709620-C279-11CE-A49E-444553540000
const GUID CLSID_Shell =
{
  0x13709620, 0xc279, 0x11ce,
  { 0xa4, 0x9e, 0x44, 0x45, 0x53, 0x54 }
};
#endif /* PLATFORM_OS_WIN32 */

std::vector<IScreenLockMethod *> System::lock_commands;

#if defined(PLATFORM_OS_UNIX)

#ifdef HAVE_DBUS_GIO
GDBusConnection* System::session_connection = NULL;
GDBusConnection* System::system_connection = NULL;
#endif

bool System::lockable = false;
bool System::shutdown_supported;

#elif defined(PLATFORM_OS_WIN32)

HINSTANCE System::user32_dll = NULL;
System::LockWorkStationFunc System::lock_func = NULL;
bool System::shutdown_supported;

#endif

bool
System::is_lockable()
{
  bool ret;
#if defined(PLATFORM_OS_UNIX)
  ret = lockable;
#elif defined(PLATFORM_OS_WIN32)
  ret = lock_func != NULL;
#else
  ret = false;
#endif
  return ret;
}

#ifdef PLATFORM_OS_UNIX
bool
System::invoke(const gchar* command, bool async)
{
  GError *error = NULL;

  if(!async)
    {
      // synchronised call
      gint exit_code;
      if (!g_spawn_command_line_sync(command, NULL, NULL, &exit_code, &error) )
        {
          g_error_free(error);
          return false;
        }
      return WEXITSTATUS(exit_code) == 0;
    }
  else
    {
      // asynchronous call
      if (!g_spawn_command_line_async(command, &error) )
        {
          g_error_free(error);
          return false;
        }
      return true;
    }
}
#endif

/* Commented out to make diff more readable
 * System::invoke, System::init_kde_lock, System::kde_lock(), System::lock()

#ifdef HAVE_DBUS_GIO
void
System::init_kde_lock()
{
  TRACE_ENTER("System::init_dbus_lock");
	GError *error = NULL;
  lock_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                             G_DBUS_PROXY_FLAGS_NONE,
                                             NULL,
                                             "org.kde.screensaver",
                                             "/ScreenSaver",
                                             "org.freedesktop.ScreenSaver",
                                             NULL,
                                             &error);

  if (error != NULL)
    {
      TRACE_MSG("Error: " << error->message);
      g_error_free(error);
    }

  if (lock_proxy != NULL)
    {
      GVariant *result = g_dbus_proxy_call_sync(lock_proxy,
                                                "GetActive",
                                                NULL,
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                &error);
      
      if (result != NULL)
        {
          g_variant_unref(result);
        }
      
      if (error != NULL)
        {
          g_error_free(error);
          g_object_unref(lock_proxy);
          lock_proxy = NULL;
        }
    }

  TRACE_EXIT();
}


bool
System::kde_lock()
{
  bool ret = false;
  
  if (lock_proxy != NULL)
    {
      GError *error = NULL;
      GVariant *result = g_dbus_proxy_call_sync(lock_proxy,
                                                "Lock",
                                                NULL,
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                &error);

      if (result != NULL)
        {
          g_variant_unref(result);
        }

      if (error != NULL)
        {
          g_error_free(error);
          ret = false;
        }
      else
        {
          ret = true;
        }
    }
  
  return ret;
}
#endif

void
System::lock()
{
  TRACE_ENTER("System::lock");
  if (is_lockable())
    {
#if defined(PLATFORM_OS_UNIX)
      gchar *program = NULL, *cmd = NULL;

#ifdef HAVE_DBUS_GIO
      if (kde_lock())
        {
          goto end;
        }
#endif
      if ((program = g_find_program_in_path("xscreensaver-command")))
        {
          cmd = g_strdup_printf("%s --display \"%s\" -lock",
                                program, lock_display.c_str() );
          if(invoke(cmd, false) )
            goto end;
        }
      if ((program = g_find_program_in_path("gnome-screensaver-command")))
        {
          cmd = g_strdup_printf("%s --lock", program);
          if(invoke(cmd, false) )
            goto end;
        }
      if ((program = g_find_program_in_path("xlock")))
        {
          cmd = g_strdup_printf("%s -display \"%s\"",
                                program, lock_display.c_str() );
          invoke(cmd, true);
        }
end:  // cleanup of created strings, jump to avoid duplication
      g_free(program);
      g_free(cmd);
#elif defined(PLATFORM_OS_WIN32)
      (*lock_func)();
#endif
    }
  TRACE_EXIT();
}
*/

#ifdef PLATFORM_OS_UNIX

#ifdef HAVE_DBUS_GIO
void
System::init_DBus()
{
  TRACE_ENTER("System::init_dbus()");
  //session_connection = workrave::CoreFactory::get_dbus()->get_connection();

  
  GError *error = NULL;
  session_connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (error != NULL)
    {
      //it is rare and serious, so report it the user
      std::cerr << "Cannot establish connection to the session bus: " << error->message << std::endl;
      g_error_free(error);
      error = NULL;
    }

  system_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
  if (error != NULL) 
    {
      std::cerr << "Cannot establish connection to the system bus: " << error->message << std::endl;
      g_error_free(error);
      error = NULL;
    }

  TRACE_EXIT();
}


inline bool System::add_DBus_lock_cmd(
    const char *dbus_name, const char *dbus_path, const char *dbus_interface,
    const char *dbus_lock_method, const char *dbus_method_to_check_existence)
{
  TRACE_ENTER_MSG("System::add_DBus_lock_cmd", dbus_name);

  // I wish we could use std::move here
  IScreenLockMethod *lock_method = NULL;
  lock_method = new ScreenLockDBus(session_connection,
          dbus_name, dbus_path, dbus_interface,
          dbus_lock_method, dbus_method_to_check_existence);
  if (!lock_method->is_lock_supported())
    {
      delete lock_method;
      lock_method = NULL;
      TRACE_RETURN(false);
      return false;
    }
  else
    {
      lock_commands.push_back(lock_method);
      TRACE_RETURN(true);
      return true;
    }

}

void
System::init_DBus_lock_commands()
{
  TRACE_ENTER("System::init_DBus_lock_commands");



  if (session_connection)
    {
      //  Unity:
      //    - Gnome screensaver API + gnome-screensaver-command works,
      //    - is going to decrease dependence and use of GNOME:
      //      https://blueprints.launchpad.net/unity/+spec/client-1311-unity7-lockscreen
      //  GNOME
      //      https://people.gnome.org/~mccann/gnome-screensaver/docs/gnome-screensaver.html#gs-method-GetSessionIdle
      //    - Gnome is now implementing the Freedesktop API, but incompletely:
      //      https://bugzilla.gnome.org/show_bug.cgi?id=689225
      //      (look for "unimplemented" in the patch), the lock method is still unipmlemented
      //    - therefore it is required to check the gnome API first.
      //WORKS: Ubuntu 12.04: GNOME 3 fallback, Unity
      add_DBus_lock_cmd(
            "org.gnome.ScreenSaver", "/org/gnome/ScreenSaver", "org.gnome.ScreenSaver",
            "Lock", "GetActive");

      //  Cinnamon:   https://github.com/linuxmint/cinnamon-screensaver/blob/master/doc/dbus-interface.html
      //    Same api as GNOME, but with different name,
      add_DBus_lock_cmd(
            "org.cinnamon.ScreenSaver", "/org/cinnamon/ScreenSaver", "org.cinnamon.ScreenSaver",
            "Lock", "GetActive");

      //  Mate: https://github.com/mate-desktop/mate-screensaver/blob/master/doc/dbus-interface.xml
      //  Like GNOME
      add_DBus_lock_cmd(
                  "org.mate.ScreenSaver", "/org/mate/ScreenSaver", "org.mate.ScreenSaver",
                  "Lock", "GetActive");


      //The FreeDesktop API - the most important and most widely supported
      //    LXDE:  https://github.com/lxde/lxqt-powermanagement/blob/master/idleness/idlenesswatcherd.cpp
      //    KDE:
      //      https://projects.kde.org/projects/kde/kde-workspace/repository/revisions/master/entry/ksmserver/screenlocker/dbus/org.freedesktop.ScreenSaver.xml
      //      - there have been claims that this does not work in some installations, but I was unable to find
      //      any traces of this in git:
      //                        http://forum.kde.org/viewtopic.php?f=67&t=111003
      //                      It was probably due to some upgrade problems (and/or a bug in KDE),
      //                      because in fresh OpenSuse 12.3 (from LiveCD) this works correctly.
      //    Razor-QT: https://github.com/Razor-qt/razor-qt/blob/master/razorqt-screenlocker/src/razorscreenlocker.cpp
      //
      //    The Freedesktop API that these DEs are implementing is being redrafted:
      //      http://people.freedesktop.org/~hadess/idle-inhibition-spec/
      //      http://lists.freedesktop.org/pipermail/xdg/2012-November/012577.html
      //      http://lists.freedesktop.org/pipermail/xdg/2013-September/012875.html
      //
      //    the Lock method there is being removed (and not replaced with anything else).
      //    Probably the DEs will support these APIs in the future in order not to break other software.

      //Is only partially implemented by GNOME, so GNOME has to go before
      //Works correctly on KDE4 (Ubuntu 12.04)
      add_DBus_lock_cmd(
            "org.freedesktop.ScreenSaver", "/ScreenSaver", "org.freedesktop.ScreenSaver",
            "Lock", "GetActive");

      //	KDE - old screensaver API - partially verified both
      add_DBus_lock_cmd(
                  "org.kde.screensaver", "/ScreenSaver", "org.freedesktop.ScreenSaver",
                  "Lock", "GetActive");
      add_DBus_lock_cmd(
                  "org.kde.krunner", "/ScreenSaver", "org.freedesktop.ScreenSaver",
                  "Lock", "GetActive");

      //              - there some accounts that when org.freedesktop.ScreenSaver does not work, this works:
      //                      qdbus org.kde.ksmserver /ScreenSaver Lock
      //                      but it is probably a side effect of the fact that implementation of org.kde.ksmserver
      //                      is in the same process as of org.freedesktop.ScreenSaver
      add_DBus_lock_cmd(
                        "org.kde.ksmserver", "/ScreenSaver", "org.freedesktop.ScreenSaver",
                        "Lock", "GetActive");

      // EFL:
      add_DBus_lock_cmd(
                  "org.enlightenment.wm.service", "/org/enlightenment/wm/RemoteObject", "org.enlightenment.wm.Desktop",
                  "Lock", NULL);
    }
  TRACE_EXIT();
}
#endif //HAVE_DBUS_GIO

inline void System::add_cmdline_lock_cmd(
        const char *command_name, const char *parameters, bool async)
{
  TRACE_ENTER_MSG("System::add_cmdline_lock_cmd", command_name);
  IScreenLockMethod *lock_method = NULL;
  lock_method = new ScreenLockCommandline(command_name, parameters, async);
  if (!lock_method->is_lock_supported())
    {
      delete lock_method;
      lock_method = NULL;
      TRACE_RETURN(false);
    }
  else
    {
      lock_commands.push_back(lock_method);
      TRACE_RETURN(true);
    }
}

void System::init_cmdline_lock_commands(const char *display)
{
  TRACE_ENTER_MSG("System::init_cmdline_lock_commands", display);

  //Works: XFCE, i3, LXDE
  add_cmdline_lock_cmd("gnome-screensaver-command", "--lock", false);
  add_cmdline_lock_cmd("mate-screensaver-command", "--lock", false);
  add_cmdline_lock_cmd("enlightenment_remote", "-desktop-lock", false);
  add_cmdline_lock_cmd("xdg-screensaver", "lock", false);

  if (display != NULL)
    {
      char *cmd = g_strdup_printf("-display \"%s\" -lock", display);
      add_cmdline_lock_cmd("xscreensaver-command", cmd, false);
      g_free(cmd);
      cmd = NULL;
    }
  else
    {
      add_cmdline_lock_cmd("xscreensaver-command", "-lock", false);
    }


  //these two may call slock, which may be not user-friendly
  //add_cmdline_lock_cmd("xflock4", NULL, true);
  //add_cmdline_lock_cmd("lxlock", NULL, true);

  if (display != NULL)
    {
      char *cmd = g_strdup_printf("-display \"%s\"", display);
      add_cmdline_lock_cmd("xlock", cmd, true);
      g_free(cmd);
      cmd = NULL;
    }
  else
    {
      add_cmdline_lock_cmd("xlock", NULL, true);
    }

  TRACE_EXIT();
}

#endif


void
System::lock()
{
  TRACE_ENTER("System::lock");

  for (std::vector<IScreenLockMethod *>::iterator iter = lock_commands.begin();
      iter != lock_commands.end(); ++iter)
    {
      if ((*iter)->lock())
        break;
    }

  TRACE_EXIT();
}

bool
System::is_shutdown_supported()
{
  bool ret;
#if defined(PLATFORM_OS_UNIX)
  ret = shutdown_supported;
#elif defined(PLATFORM_OS_WIN32)
  ret = shutdown_supported;
#else
  ret = false;
#endif
  return ret;
}

void
System::shutdown()
{
#if defined(PLATFORM_OS_UNIX)
  gchar *program = NULL, *cmd = NULL;

  if ((program = g_find_program_in_path("gnome-session-save")))
    {
      cmd = g_strdup_printf("%s --kill", program);
      invoke(cmd, false);
    }
#elif defined(PLATFORM_OS_WIN32)
  shutdown_helper(true);
#endif
}

#ifdef PLATFORM_OS_WIN32
bool
System::shutdown_helper(bool for_real)
{
  bool ret = false;
  IShellDispatch* pShellDispatch = NULL;
  if (SUCCEEDED(::CoCreateInstance(CLSID_Shell, NULL, CLSCTX_SERVER,
                                   IID_IShellDispatch,
                                   (LPVOID*)&pShellDispatch)))
    {
      ret = true;
      if (for_real)
        {
          harpoon_unblock_input();
          pShellDispatch->ShutdownWindows();
        }
      pShellDispatch->Release();
    }
  return ret;
}
#endif

void
System::init(
#if defined(PLATFORM_OS_UNIX)
             const char *display
#endif
             )
{
  TRACE_ENTER("System::init");
#if defined(PLATFORM_OS_UNIX)
#ifdef HAVE_DBUS_GIO
  init_DBus();
  init_DBus_lock_commands();
#endif
  init_cmdline_lock_commands(display);

  lockable = !lock_commands.empty();
  if (lockable)
    {
      TRACE_MSG("Locking enabled");
    }
  else
    {
      TRACE_MSG("Locking disabled");
    }

  shutdown_supported = false;
  if (!shutdown_supported && (g_find_program_in_path("gnome-session-save") != NULL))
    {
      shutdown_supported = true;
    }

#elif defined(PLATFORM_OS_WIN32)
  // Note: this memory is never freed
  user32_dll = LoadLibrary("user32.dll");
  if (user32_dll != NULL)
    {
      lock_func = (LockWorkStationFunc)
        GetProcAddress(user32_dll, "LockWorkStation");
    }
  shutdown_supported = shutdown_helper(false);
#endif

  TRACE_EXIT();
}

void
System::clear()
{
  for (std::vector<IScreenLockMethod *>::iterator iter = lock_commands.begin();
        iter != lock_commands.end(); ++iter)
    {
      delete *iter;
    }
  lock_commands.clear();
#ifdef HAVE_DBUS_GIO
  //we should call g_dbus_connection_close_sync here:
  //http://comments.gmane.org/gmane.comp.freedesktop.dbus/15286
  g_object_unref(session_connection);
  g_object_unref(session_connection);
#endif
}
//TODO: lock before suspend/hibernate

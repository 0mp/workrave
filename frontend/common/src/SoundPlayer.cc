// SoundPlayer.cc --- Sound player
//
// Copyright (C) 2002, 2003, 2004, 2006, 2007 Rob Caelers & Raymond Penners
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

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SoundPlayer.hh"
#include "Thread.hh"
#include "Sound.hh"
#include "debug.hh"

#include "IConfigurator.hh"
#include "CoreFactory.hh"

#ifdef HAVE_GNOME
#include <gdk/gdk.h>
#include "GnomeSoundPlayer.hh"
#endif
#ifdef HAVE_KDE
#include "KdeSoundPlayer.hh"
#endif
#ifdef PLATFORM_OS_UNIX
#include <X11/Xlib.h>
#endif
#ifdef PLATFORM_OS_WIN32
#include <windows.h>
#include "W32SoundPlayer.hh"
#endif
#ifdef PLATFORM_OS_OSX
#include "OSXSoundPlayer.hh"
#endif

const char *SoundPlayer::CFG_KEY_SOUND_ENABLED = "sound/enabled";
const char *SoundPlayer::CFG_KEY_SOUND_DEVICE = "sound/device";

using namespace workrave;
using namespace std;

/**********************************************************************
 * PC-Speaker
 **********************************************************************/
static short prelude_beeps[][2]=
{
    { 250, 50},
    { 300, 50},
    { 0, 0 }
};

static short micro_break_start_beeps[][2]=
{
    { 320, 70 },
    { 350, 70 },
    { 0, 0 },
};

static short micro_break_end_beeps[][2]=
{
  { 350, 70 },
  { 320, 70 },
  { 0, 0 },
};

static short rest_break_start_beeps[][2]=
{
  { 160, 70 },
  { 180, 70 },
  { 200, 70 },
  { 230, 70 },
  { 260, 70 },
  { 290, 70 },
  { 320, 70 },
  { 350, 70 },
  { 0, 0 }
};

static short rest_break_end_beeps[][2]=
{
  { 350, 70 },
  { 320, 70 },
  { 290, 70 },
  { 260, 70 },
  { 230, 70 },
  { 200, 70 },
  { 180, 70 },
  { 160, 70 },
  { 0, 0 }
};

static short break_ignore_beeps[][2]=
{
    { 60, 250 },
    { 50, 400 },
    { 0, 0 }
};

static short daily_limit_beeps[][2]=
{
    { 80, 200 },
    { 70, 200 },
    { 60, 200 },
    { 50, 400 },
    { 0, 0 }
};


static short exercise_ended_beeps[][2]=
{
    { 320, 70 },
    { 350, 70 },
    { 0, 0 },
};


static short exercises_ended_beeps[][2]=
{
    { 350, 70 },
    { 320, 70 },
    { 0, 0 },
};

static short (*beep_map[])[2] =
{
  prelude_beeps,
  break_ignore_beeps,
  rest_break_start_beeps,
  rest_break_end_beeps,
  micro_break_start_beeps,
  micro_break_end_beeps,
  daily_limit_beeps,
  exercise_ended_beeps,
  exercises_ended_beeps
};


class SpeakerPlayer : public Thread
{
public:
  SpeakerPlayer(short (*beeps)[2]);
  void run();
private:
  SoundPlayer *player;
  short (*beeps)[2];
};


SpeakerPlayer::SpeakerPlayer(short (*b)[2])
  : Thread(true)
{
  beeps = b;
}

void
SpeakerPlayer::run()
{
  TRACE_ENTER("SpeakerPlayer::run");
#ifdef PLATFORM_OS_WIN32
  // Windows 95 Beep() only beeps, it ignores frequency & duration parameters.
  // So, in the case of W95 do not relay on Sound::beep()
  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  if (! GetVersionEx(&osvi))
    return;
  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
      && osvi.dwMinorVersion == 0) {
    ::Beep(256, 256);
    return;
  }
#endif

  short (*b)[2];
  b = beeps;
#ifdef PLATFORM_OS_UNIX
  Display *display = NULL;
#  ifdef HAVE_GNOME
  display = XOpenDisplay(gdk_get_display());
#  endif
  if (display) {
#endif
  while (b[0][0])
    {
#ifdef PLATFORM_OS_UNIX
      Sound::beep(display, b[0][0], b[0][1]);
#else
      Sound::beep(b[0][0], b[0][1]);
#endif
      b++;
    }
#ifdef PLATFORM_OS_UNIX
    XCloseDisplay(display);
  }
#endif
  TRACE_EXIT();
}


/**********************************************************************
 * SoundPlayer
 **********************************************************************/


SoundPlayer::SoundPlayer()
{
  player =
#if defined(PLATFORM_OS_WIN32)
     new W32SoundPlayer()
#elif defined(HAVE_GNOME)
     new GnomeSoundPlayer()
#elif defined(HAVE_KDE)
     new KdeSoundPlayer()
#elif defined(PLATFORM_OS_OSX)
     new OSXSoundPlayer()
#else
#  warning Sound card support disabled.
     NULL
#endif
    ;
}

SoundPlayer::~SoundPlayer()
{
  delete player;
}

void
SoundPlayer::play_sound(Sound snd)
{
  TRACE_ENTER("SoundPlayer::play_sound");
  if (is_enabled())
    {
      if (get_device() == DEVICE_SOUNDCARD && player != NULL)
        {
          player->play_sound(snd);
        }
      else
        {
          Thread *t = new SpeakerPlayer(beep_map[snd]);
          t->start();
        }
    }
  TRACE_EXIT();
}


bool
SoundPlayer::is_enabled()
{
  bool b;
  bool rc;
  b = CoreFactory::get_configurator()
    ->get_value(CFG_KEY_SOUND_ENABLED, rc);
  if (! b)
    {
      rc = true;
    }
  return rc;
}

void
SoundPlayer::set_enabled(bool b)
{
  CoreFactory::get_configurator()
    ->set_value(CFG_KEY_SOUND_ENABLED, b);
}

SoundPlayer::Device
SoundPlayer::get_device()
{
  bool b;
  Device dev = DEVICE_SOUNDCARD;
  string devstr;
  b = CoreFactory::get_configurator()
    ->get_value(CFG_KEY_SOUND_DEVICE, devstr);
  if (b)
    {
      if (devstr == "speaker")
        {
          dev = DEVICE_SPEAKER;
        }
    }
  return dev;
}

void
SoundPlayer::set_device(Device dev)
{
  const char *devstr;
  switch (dev)
    {
    case DEVICE_SPEAKER:
      devstr = "speaker";
      break;
    default:
      devstr = "soundcard";
      break;
    }
  CoreFactory::get_configurator()
    ->set_value(CFG_KEY_SOUND_DEVICE, string(devstr));
}

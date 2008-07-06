// OSXSoundPlayer.cc --- Sound player
//
// Copyright (C) 2002 - 2008 Raymond Penners & Ray Satiro
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

static const char rcsid[] = "$Id: OSXSoundPlayer.cc 1275 2007-08-16 21:30:44Z rcaelers $";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"
#include <strings.h>

#include "OSXSoundPlayer.hh"
#include "SoundPlayer.hh"
#include "Util.hh"

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

OSXSoundPlayer::OSXSoundPlayer()
{
  EnterMovies();
}


OSXSoundPlayer::~OSXSoundPlayer()
{
}

bool
OSXSoundPlayer::capability(SoundPlayer::SoundCapability cap)
{
  if (cap == SoundPlayer::SOUND_CAP_EDIT)
    {
      return true;
    }
  return false;
}


void
OSXSoundPlayer::play_sound(SoundPlayer::SoundEvent snd)
{
  (void) snd;
}


void
OSXSoundPlayer::play_sound(string file)
{
  if (wav_file == NULL)
    {
      wav_file = strdup(file.c_str());
      start();
    }
}


void
OSXSoundPlayer::run()
{
  OSErr err;
  FSSpec spec;
  const char *fname = wav_file;
  Movie movie;

  FSRef fref;
  err = FSPathMakeRef((const UInt8 *) fname, &fref, NULL);
  if (err != noErr)
    {
      printf("FSPathMakeRef failed %d\n", err);
      return;
    }
  err = FSGetCatalogInfo(&fref, kFSCatInfoNone, NULL, NULL, &spec, NULL);
  if (err != noErr)
    {
      printf("FSGetCatalogInfo failed %d\n", err);
      return;
    }

  short movieResFile = 0;
  err = OpenMovieFile(&spec, &movieResFile, fsRdPerm);
  if (err != noErr)
    {
      printf("OpenMovieFile failed %d\n", err);
      return;
    }

  Str255 name;
  err = NewMovieFromFile(&movie, movieResFile, NULL, name, 0, NULL);
  if (err != noErr)
    {
      printf("NewMovieFromFile failed %d\n", err);
      err = CloseMovieFile(movieResFile);
      if (err != noErr)
        {
          printf("CloseMovieFile failed %d\n", err);
        }
      return;
    }


  // play the movie once thru
  StartMovie(movie);

  while (!IsMovieDone(movie))
    {
      MoviesTask(movie, 0);
    }

  DisposeMovie(movie);
  free((void*)wav_file);
  wav_file = NULL;
}


bool OSXSoundPlayer::get_sound_enabled(SoundPlayer::SoundEvent snd, bool &enabled)
{
  (void) snd;
  (void) enabled;
  return false;
}

void OSXSoundPlayer::set_sound_enabled(SoundPlayer::SoundEvent snd, bool enabled)
{
  (void) snd;
  (void) enabled;
}

bool OSXSoundPlayer::get_sound_wav_file(SoundPlayer::SoundEvent snd, std::string &wav_file)
{
  (void) snd;
  (void) wav_file;
  return false;
}

void OSXSoundPlayer::set_sound_wav_file(SoundPlayer::SoundEvent snd, const std::string &wav_file)
{
  (void) snd;
  (void) wav_file;
}


// OSXSoundPlayer.hh
//
// Copyright (C) 2007, 2008, 2009, 2010, 2013 Rob Caelers <robc@krandor.nl>
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

#ifndef OSXSOUNDPLAYER_HH
#define OSXSOUNDPLAYER_HH

#include "audio/ISoundDriver.hh"
#include "Thread.hh"

class OSXSoundPlayer : public ISoundDriver, public Thread
{
public:
  OSXSoundPlayer();
  virtual ~OSXSoundPlayer();

  void init(ISoundDriverEvents *) {};
  bool capability(SoundCapability cap);
  void play_sound(std::string wavfile, int volume);
  void play_sound(SoundEvent snd, int volume);

private:

  void run();

  const char *wav_file;
};

#endif // OSXSOUNDPLAYER_HH

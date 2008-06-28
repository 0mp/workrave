// OSXSoundPlayer.hh
//
// Copyright (C) 2007, 2008 Rob Caelers <robc@krandor.nl>
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
// $Id: OSXSoundPlayer.hh 1230 2007-07-03 15:59:16Z rcaelers $
//

#ifndef OSXSOUNDPLAYER_HH
#define OSXSOUNDPLAYER_HH

#include "ISoundDriver.hh"

class OSXSoundPlayer : public ISoundDriver
{
public:
  OSXSoundPlayer();
  virtual ~OSXSoundPlayer();

  bool capability(SounCapability cap);
  void play_sound(string wavfile);
  void play_sound(SoundEvent snd);

private:
};

#endif // OSXSOUNDPLAYER_HH

// W32SoundPlayer.hh
//
// Copyright (C) 2002, 2003, 2006, 2007 Raymond Penners & Ray Satiro
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
// $Id$
//

#ifndef W32SOUNDPLAYER_HH
#define W32SOUNDPLAYER_HH

#include "ISoundPlayer.hh"

class W32SoundPlayer : public ISoundPlayer
{
public:
  W32SoundPlayer();
  virtual ~W32SoundPlayer();

  void play_sound(Sound snd);

protected:
  static DWORD WINAPI thread_Play( LPVOID );

private:
  void register_sound_events();
  void Play();
};

#endif // W32SOUNDPLAYER_HH

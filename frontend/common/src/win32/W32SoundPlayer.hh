// W32SoundPlayer.hh
//
// Copyright (C) 2002, 2003, 2006 Raymond Penners & Ray Satiro
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
  //static DWORD WINAPI thread_proc(LPVOID lpParameter);
  //static volatile HANDLE thread_handle;
};

#endif // W32SOUNDPLAYER_HH

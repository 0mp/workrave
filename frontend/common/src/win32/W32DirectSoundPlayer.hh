// W32DirectSoundPlayer.hh
//
// Copyright (C) 2002, 2003, 2006, 2007, 2008, 2009 Raymond Penners & Ray Satiro
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

#ifndef W32DIRECTSOUNDPLAYER_HH
#define W32DIRECTSOUNDPLAYER_HH

#include "ISoundDriver.hh"

#include <windows.h>
#include <dxerr9.h>
#include <dsound.h>
#include <string>

class W32DirectSoundPlayer : public ISoundDriver
{
public:
  W32DirectSoundPlayer();
  virtual ~W32DirectSoundPlayer();

  void init();
  bool capability(SoundPlayer::SoundCapability cap);
  void play_sound(SoundPlayer::SoundEvent snd);
  void play_sound(std::string wavfile);

  bool get_sound_enabled(SoundPlayer::SoundEvent snd, bool &enabled);
  void set_sound_enabled(SoundPlayer::SoundEvent snd, bool enabled);
  bool get_sound_wav_file(SoundPlayer::SoundEvent snd, std::string &wav_file);
  void set_sound_wav_file(SoundPlayer::SoundEvent snd, const std::string &wav_file);

private:
  static DWORD WINAPI play_thread(LPVOID);

  void play();
    
private:
  LPDIRECTSOUND8 direct_sound;
};


class WaveFile
{
public:
  WaveFile(const std::string &filename);
  ~WaveFile();

  void init();

  size_t read(BYTE *buffer, size_t size);
  size_t get_size();
  
  void  reset_file();
  WAVEFORMATEX *get_format() { return wfx; };

private:
  std::string filename;
  HMMIO mmio;
  WAVEFORMATEX format;
  MMCKINFO child;
  MMCKINFO parent;
  WAVEFORMATEX *wfx;
  DWORD sample_size;
};


class SoundClip
{
public:
  SoundClip(LPDIRECTSOUND8 direct_sound, const std::string &filename);
  virtual ~SoundClip();

  void init();
  void play();

private:

  void fill_buffer();
  bool is_buffer_lost();
  void restore_buffer();

private:
  std::string filename;
  LPDIRECTSOUND8 direct_sound;
  WaveFile *wave_file;
  LPDIRECTSOUNDBUFFER sound_buffer;
  DWORD sound_buffer_size;
};


#endif // W32DIRECTSOUNDPLAYER_HH

// SoundTheme.hh
//
// Copyright (C) 2002 - 2013 Rob Caelers & Raymond Penners
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

#ifndef SOUNDTHEME_HH
#define SOUNDTHEME_HH

#include <string>
#include <list>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>

#include "config/IConfigurator.hh"
#include "audio/ISoundPlayer.hh"

class SoundTheme
{
public:
  class Theme
  {
  public:
    std::string description;
    std::vector<std::string> files;
    bool active;
  };
  
  struct SoundRegistry
  {
    const char *id;
    const char *friendly_name;
  };

  typedef boost::shared_ptr<SoundTheme> Ptr;
      
  static Ptr create(workrave::config::IConfigurator::Ptr config);
  
  SoundTheme(workrave::config::IConfigurator::Ptr config);
  virtual ~SoundTheme();

  void init();

  void play_sound(workrave::audio::SoundEvent snd, bool mute_after_playback = false);
  void play_sound(std::string wavfile);
  void restore_mute();
  bool capability(workrave::audio::SoundCapability cap);
  
  bool is_enabled();
  void set_enabled(bool enabled);

  bool get_sound_enabled(workrave::audio::SoundEvent snd);
  void set_sound_enabled(workrave::audio::SoundEvent snd, bool enabled);
  std::string get_sound_wav_file(workrave::audio::SoundEvent snd);
  void set_sound_wav_file(workrave::audio::SoundEvent snd, const std::string &wav_file);

  void get_sound_themes(std::vector<Theme> &themes);
  void load_sound_theme(const std::string &path, Theme &theme);
  void activate_theme(const Theme &theme, bool force = true);

#ifdef PLATFORM_OS_WIN32  
  void win32_remove_deprecated_appevents();
#endif
  
private:
  void register_sound_events(std::string theme = "");

public:
  static SoundRegistry sound_registry[workrave::audio::SOUND_MAX];

private:
  workrave::config::IConfigurator::Ptr config;
  workrave::audio::ISoundPlayer::Ptr player;
};

#endif // SOUNDTHEME_HH

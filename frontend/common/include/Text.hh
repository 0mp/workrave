// Text.hh
//
// Copyright (C) 2002, 2007 Raymond Penners <raymond@dotsphinx.com>
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

#ifndef TEXT_HH
#define TEXT_HH

#include <string>
#include <time.h>

class Text
{
public:
  static std::string time_to_string(time_t t, bool display_units = false);
};

#endif // TEXT_HH

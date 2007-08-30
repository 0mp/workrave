// TimePredFactory.hh
//
// Copyright (C) 2001, 2002 Rob Caelers <robc@krandor.org>
// All rights reserved.
//
// Time-stamp: <2007-08-30 16:03:08 robc>
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

#ifndef TIMEPREDFACTORY_HH
#define TIMEPREDFACTORY_HH

#include <string>

class TimePred;

class TimePredFactory
{
public:
  static TimePred *create_time_pred(std::string type);
};

#endif // TIMEPREDFACTORY_HH

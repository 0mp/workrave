// StatisticsInterface.hh
//
// Copyright (C) 2002, 2003, 2005 Rob Caelers & Raymond Penners
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

#ifndef STATISTICSINTERFACE_HH
#define STATISTICSINTERFACE_HH

#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>

#include "CoreInterface.hh"

class StatisticsInterface
{
public:
  enum StatsBreakValueType
    {
      STATS_BREAKVALUE_PROMPTED = 0,
      STATS_BREAKVALUE_TAKEN,
      STATS_BREAKVALUE_NATURAL_TAKEN,
      STATS_BREAKVALUE_SKIPPED,
      STATS_BREAKVALUE_POSTPONED,
      STATS_BREAKVALUE_UNIQUE_BREAKS,
      STATS_BREAKVALUE_TOTAL_OVERDUE,
      STATS_BREAKVALUE_SIZEOF
    };

  enum StatsValueType
    {
      STATS_VALUE_TOTAL_ACTIVE_TIME = 0,
      STATS_VALUE_TOTAL_MOUSE_MOVEMENT,
      STATS_VALUE_TOTAL_CLICK_MOVEMENT,
      STATS_VALUE_TOTAL_MOVEMENT_TIME,
      STATS_VALUE_TOTAL_CLICKS,
      STATS_VALUE_TOTAL_KEYSTROKES,
      STATS_VALUE_SIZEOF
    };

  typedef int BreakStats[STATS_BREAKVALUE_SIZEOF];
  typedef int MiscStats[STATS_VALUE_SIZEOF];
  
  struct DailyStats
  {
    //! Start time of this day.
    struct tm start;

    //! Stop time of this day.
    struct tm stop;

    //! Statistic of each break
    BreakStats break_stats[BREAK_ID_SIZEOF];

    //! Misc statistics
    MiscStats misc_stats;
  };
  
public:
  virtual void update() = 0;
  virtual DailyStats *get_current_day() const = 0;
  virtual DailyStats *get_day(int day) const = 0;
  virtual void get_day_index_by_date(int y, int m, int d, int &idx, int &next, int &prev) const = 0;
  virtual int get_history_size() const = 0;
  virtual void dump() = 0;
};

#endif // STATISTICSINTERFACE_HH

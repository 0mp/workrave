// NetworkLogDialog.hh --- Network Log Dialog
//
// Copyright (C) 2002, 2003 Raymond Penners <raymond@dotsphinx.com>
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

#ifndef NETWORKLOGDIALOG_HH
#define NETWORKLOGDIALOG_HH

#include <stdio.h>

#include "preinclude.h"

class TimeEntry;

#include <gtkmm.h>
#include "DistributionLogListener.hh"

class NetworkLogDialog :
  public Gtk::Dialog,
  public DistributionLogListener
{
public:  
  NetworkLogDialog();
  ~NetworkLogDialog();

  int run();
  
private:
  void init();
  void distribution_log(string msg);
  void on_response(int response);
  
  Gtk::TextView *text_view;
  Gtk::ScrolledWindow scrolled_window;
  Glib::RefPtr<Gtk::TextBuffer> text_buffer;  
};

#endif // NETWORKLOGWINDOW_HH

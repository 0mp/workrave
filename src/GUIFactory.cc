// GUIFactory.cc
//
// Copyright (C) 2001, 2002 Rob Caelers <robc@krandor.org>
// All rights reserved.
//
// Time-stamp: <2002-10-15 09:45:51 robc>
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

static const char rcsid[] = "$Id$";

#include "GUIFactory.hh"
#include "gtkmm/GtkmmGUI.hh"

GUIInterface *
GUIFactory::create_gui(string type, ControlInterface *c, int argc, char **argv)
{
  GUIInterface *gi =  NULL;
  
  if (type == "gtkmm")
    {
      gi =GtkmmGUI::create(c, argc, argv);

    }
  
  return gi;
}

  

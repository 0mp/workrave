// EventLabel.cc ---
//
// Copyright (C) 2003 Rob Caelers <robc@krandor.org>
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

static const char rcsid[] = "$Id$";

#include "preinclude.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.hh"

#include "EventLabel.hh"


void
EventLabel::on_realize()
{
  GtkWidget *widget = GTK_WIDGET(gobj());

  GdkWindowAttr attributes;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x ;
  attributes.y = widget->allocation.y ;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events(widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
                            GDK_BUTTON_MOTION_MASK |
                            GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_ENTER_NOTIFY_MASK |
                            GDK_LEAVE_NOTIFY_MASK);

  gint attributes_mask = GDK_WA_X | GDK_WA_Y;

  Gtk::Label::on_realize();

  event_window = gdk_window_new(gtk_widget_get_parent_window(widget),
                                &attributes, attributes_mask);

  gdk_window_set_user_data(event_window, widget);
  gdk_window_show(event_window);
}

void
EventLabel::on_unrealize()
{
  if (event_window)
    {
      gdk_window_set_user_data(event_window, NULL);
      gdk_window_destroy(event_window);
      event_window = NULL;
    }
  
  Gtk::Label::on_unrealize();
}

bool
EventLabel::on_map_event(GdkEventAny *event)
{
  Gtk::Label::on_map_event(event);
  
  if (event_window)
    {
      gdk_window_show(event_window);
    }
}

bool
EventLabel::on_unmap_event(GdkEventAny *event)
{
  if (event_window)
    {
      gdk_window_hide(event_window);
    }
  
  Gtk::Label::on_unmap_event(event);
}

void
EventLabel::on_size_allocate(GtkAllocation *allocation)
{
  Gtk::Label::on_size_allocate(allocation);
  
  GtkWidget *widget = GTK_WIDGET(gobj());

  if (GTK_WIDGET_REALIZED(widget))
    {
      gdk_window_move_resize(event_window,
                             allocation->x ,
                             allocation->y ,
                             allocation->width,
                             allocation->height);
    }
}

// KWorkraveApplet.hh --- KDE Workrave Applet
//
// Copyright (C) 2004 Rob Caelers
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

#ifndef KWORKRAVEAPPLET_H
#define KWORKRAVEAPPLET_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dcopobject.h>
#include <kpanelapplet.h>
#include <qxembed.h>
#include <kcmodule.h>
#include <kconfig.h>

class KWinModule;

class KWorkraveApplet : public KPanelApplet, public DCOPObject
{
  Q_OBJECT
  K_DCOP
  
k_dcop:
  void embed_window(int window_id);
  void set_size(int width, int height);
  long get_size();
  bool get_vertical();

public:
  KWorkraveApplet(const QString& configFile, Type t = Normal, int actions = 0,
                  QWidget *parent = 0, const char *name = 0);
  virtual ~KWorkraveApplet();

protected slots:
  void embedded_window_destroyed();

private:
  QXEmbed *embed;
  KWinModule *kwin_module;
};

#endif

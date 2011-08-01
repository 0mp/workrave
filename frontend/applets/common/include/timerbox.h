/*
 * workrave-timerbox.h
 *
 * Copyright (C) 2011 Rob Caelers <robc@krandor.nl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WORKRAVE_TIMERBOX_H__
#define __WORKRAVE_TIMERBOX_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "timebar.h"

#define WORKRAVE_TYPE_TIMERBOX                  (workrave_timerbox_get_type())
#define WORKRAVE_TIMERBOX(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), WORKRAVE_TYPE_TIMERBOX, WorkraveTimerbox))
#define WORKRAVE_IS_TIMERBOX(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), WORKRAVE_TYPE_TIMERBOX))
#define WORKRAVE_TIMERBOX_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST((klass), WORKRAVE_TYPE_TIMERBOX, WorkraveTimerboxClass))
#define WORKRAVE_IS_TIMERBOX_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE((klass), WORKRAVE_TYPE_TIMERBOX))
#define WORKRAVE_TIMERBOX_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), WORKRAVE_TYPE_TIMERBOX, WorkraveTimerboxClass))

typedef struct _WorkraveTimerbox        WorkraveTimerbox;
typedef struct _WorkraveTimerboxClass   WorkraveTimerboxClass;
typedef struct _WorkraveTimerboxPrivate WorkraveTimerboxPrivate;

struct _WorkraveTimerbox
{
  GObject parent_instance;

  /*< private >*/
  WorkraveTimerboxPrivate *priv;
};

struct _WorkraveTimerboxClass
{
  GObjectClass parent_class;
};

GType workrave_timerbox_get_type(void);


typedef enum WorkraveBreakId
  {
    BREAK_ID_NONE = -1,
    BREAK_ID_MICRO_BREAK = 0,
    BREAK_ID_REST_BREAK,
    BREAK_ID_DAILY_LIMIT,
    BREAK_ID_SIZEOF
  } WorkraveBreakId;


/*
 * Method definitions.
 */


/**
  * workrave_timerbox_update: 
  *
  * @self: a @WorkraveTimerbox 
  * @image: a @GtkImage where the timerbox will be drawn into
  *
  */
void workrave_timerbox_update(WorkraveTimerbox *self, GtkImage *image);

/**
  * workrave_timerbox_set_slot: 
  *
  * @self: a @WorkraveTimerbox
  * @slot:
  * @brk: 
  *
  */
void workrave_timerbox_set_slot(WorkraveTimerbox *self, int slot, WorkraveBreakId brk);

/**
  * workrave_timerbox_set_enabled: 
  *
  * @self: a @WorkraveTimerbox 
  * @enabled: 
  *
  */
void workrave_timerbox_set_enabled(WorkraveTimerbox *self, gboolean enabled);

/**
  * workrave_timerbox_get_time_bar: 
  *
  * @self: a @WorkraveTimerbox 
  * @timer: the ID of the #WorkraveTimebar to return
  *
  * Return value: (transfer none): The @WorkraveTimebar of the specified timer
  *
  */
WorkraveTimebar *workrave_timerbox_get_time_bar(WorkraveTimerbox *self, WorkraveBreakId timer);

#endif /* __WORKRAVE_TIMERBOX_H__ */

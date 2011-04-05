/*
 * This file is part of gdspeak
 *
 * Copyright (C) 2011 Kaj-Michael Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef _GDBUSSPEAK_H
#define _GDBUSSPEAK_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GDBUSSPEAK_TYPE			(gdbusspeak_get_type())
#define GDBUSSPEAK(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDBUSSPEAK_TYPE, Gdbusspeak))
#define GDBUSSPEAK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GDBUSSPEAK_TYPE, GdbusspeakClass))
#define IS_GDBUSSPEAK(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDBUSSPEAK_TYPE))
#define IS_GDBUSSPEAK_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GDBUSSPEAK_TYPE))
#define GDBUSSPEAK_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GDBUSSPEAK_TYPE, GdbusspeakClass))

#define GDBUSSPEAK_NAME_DBUS "org.tal.gdspeak"
#define GDBUSSPEAK_PATH_DBUS "/org/tal/gdspeak"
#define GDBUSSPEAK_INTERFACE_DBUS "org.tal.gdspeak"

/** 
 * gdbusspeak GObject, extends from Gdspeak
 */
typedef struct _Gdbusspeak Gdbusspeak;
struct _Gdbusspeak {
	Gdspeak parent;
	gpointer priv;
};

typedef struct _GdbusspeakClass GdbusspeakClass;
struct _GdbusspeakClass {
	GdspeakClass parent;
};

GType gdbusspeak_get_type(void);
Gdbusspeak *gdbusspeak_new(void);

G_END_DECLS

#endif

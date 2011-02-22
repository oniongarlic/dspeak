/*
 * This file is part of dspeak
 *
 * Copyright (C) 2011 Kaj-Michael Lang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _GDSPEAK_H
#define _GDSPEAK_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GDSPEAK_TYPE			(gdspeak_get_type())
#define GDSPEAK(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDSPEAK_TYPE, Gdspeak))
#define GDSPEAK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GDSPEAK_TYPE, GdspeakClass))
#define IS_GDSPEAK(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDSPEAK_TYPE))
#define IS_GDSPEAK_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GDSPEAK_TYPE))
#define GDSPEAK_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GDSPEAK_TYPE, GdspeakClass))

/** 
 * dspeak GObject
 */
typedef struct _Gdspeak Gdspeak;
struct _Gdspeak {
	GObject parent;
	gpointer priv;
};

typedef struct _GdspeakClass GdspeakClass;
struct _GdspeakClass {
	GObjectClass parent;
};

GType gdspeak_get_type(void);

Gdspeak *gdspeak_new(void);
gboolean gdspeak_speak(Gdspeak *gds, const gchar *txt);
gboolean gdspeak_speak_priority(Gdspeak *gds, guint priority, const gchar *txt);
gboolean gdspeak_speak_full(Gdspeak *gds, const gchar *txt, gint speed, gint tone, const gchar *lang);
gboolean gdspeak_stop(Gdspeak *gds);

G_END_DECLS

#endif

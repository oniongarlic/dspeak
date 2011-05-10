/*
 * This file is part of gdspeak
 *
 * Copyright (C) 2011 Kaj-Michael Lang
 *
 *  gdspeak is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gdspeak is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with gdspeak.  If not, see <http://www.gnu.org/licenses/>.
 *
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

guint32 gdspeak_speak_full(Gdspeak *gs, const gchar *txt, const gchar *lang, guint priority, gint pitch, gint range, gint rate, gint volume);
guint32 gdspeak_speak_priority(Gdspeak *gds, guint priority, const gchar *txt);
gboolean gdspeak_speak(Gdspeak *gds, const gchar *txt);

gboolean gdspeak_stop(Gdspeak *gds, gboolean clear);

void gdspeak_clear(Gdspeak *gs);

gboolean gdspeak_speaking(Gdspeak *gds);

gboolean gdspeak_set_voice(Gdspeak *gs, const gchar *voice);
gboolean gdspeak_set_default_voice(Gdspeak *gs, const gchar *voice);
GHashTable *gdspeak_get_voice(Gdspeak *gs);

guint gdspeak_voices(Gdspeak *gs);
GHashTable *gdspeak_list_voices(Gdspeak *gs);

G_END_DECLS

#endif

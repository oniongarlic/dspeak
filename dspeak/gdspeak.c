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

#include <glib.h>
#include <glib-object.h>
#include <espeak/speak_lib.h>

#include "gdspeak.h"
#include "speak.h"

#include "dspeak-server-glue.h"

#define DEFAULT_LANG "en"

G_DEFINE_TYPE(Gdspeak, gdspeak, G_TYPE_OBJECT);

/* Signal IDs */
enum {
	SPOKEN,
	LAST_SIGNAL
};

/* Property IDs */
enum {
	PROP_0,
	PROP_PITCH,
	PROP_RANGE,
	PROP_RATE,
	PROP_VOLUME,
};

typedef struct _GdspeakPrivate GdspeakPrivate;
struct _GdspeakPrivate {
	GQueue *sentences;
	GSList *voices;
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDSPEAK_TYPE, GdspeakPrivate))

static guint signals[LAST_SIGNAL]={ 0 };

static void
gdspeak_dispose(GObject *object)
{
G_OBJECT_CLASS(gdspeak_parent_class)->dispose(object);
}

static void
gdspeak_finalize(GObject *object)
{
Gdspeak *go=GDSPEAK(object);

g_return_if_fail(go);
G_OBJECT_CLASS(gdspeak_parent_class)->finalize(object);
}

static void
gdspeak_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
Gdspeak *go=GDSPEAK(object);

switch (prop_id) {
	case PROP_PITCH:
		espeak_SetParameter(espeakPITCH, g_value_get_int(value), 0);
	break;
	case PROP_RATE:
		espeak_SetParameter(espeakRATE, g_value_get_int(value), 0);
	break;
	case PROP_RANGE:
		espeak_SetParameter(espeakRANGE, g_value_get_int(value), 0);
	break;
	case PROP_VOLUME:
		espeak_SetParameter(espeakVOLUME, g_value_get_int(value), 0);
	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
}
}

static void
gdspeak_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
Gdspeak *go=GDSPEAK(object);

switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
}
}

static GObject*
gdspeak_constructor(GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
static GObject *self=NULL;

if (self==NULL) {
	self=G_OBJECT_CLASS(gdspeak_parent_class)->constructor (type, n_construct_params, construct_params);
	g_object_add_weak_pointer(self, (gpointer) &self);
	return self;
}
return g_object_ref(self);
}

static void
gdspeak_class_init(GdspeakClass *klass)
{
GParamSpec *pspec;
GObjectClass *object_class=G_OBJECT_CLASS(klass);

object_class->constructor=gdspeak_constructor;
object_class->dispose=gdspeak_dispose;
object_class->finalize=gdspeak_finalize;
object_class->set_property=gdspeak_set_property;
object_class->get_property=gdspeak_get_property;

dbus_g_object_type_install_info(gdspeak_get_type (), &dbus_glib_gdspeak_object_info);

g_type_class_add_private(object_class, sizeof(GdspeakPrivate));

pspec=g_param_spec_uint("pitch", "Pitch", "Speech base pitch, range 0-100.  50=normal", 0, 100, 50, G_PARAM_WRITABLE);
g_object_class_install_property(object_class, PROP_PITCH, pspec);

pspec=g_param_spec_uint("range", "Range", "Pitch range, range 0-100. 0-monotone, 50=normal", 0, 100, 50, G_PARAM_WRITABLE);
g_object_class_install_property(object_class, PROP_RANGE, pspec);

pspec=g_param_spec_uint("rate", "Rate", "Speech speed, in words per minute", espeakRATE_MINIMUM, espeakRATE_MAXIMUM, espeakRATE_NORMAL, G_PARAM_WRITABLE);
g_object_class_install_property(object_class, PROP_RATE, pspec);

pspec=g_param_spec_uint("volume", "Volume", "Speech volume", 0, 100, 50, G_PARAM_WRITABLE);
g_object_class_install_property(object_class, PROP_VOLUME, pspec);

speak_init(DEFAULT_LANG);
}

static void 
gdspeak_init(Gdspeak *gs)
{
GdspeakPrivate *p;
espeak_VOICE **vs, **i;

p=GET_PRIVATE(gs);

p->sentences=g_queue_new();

vs=espeak_ListVoices(NULL);

for (i=vs; *i; i++) {
	espeak_VOICE *v=*i;

	g_debug("V: [%s] (%s) [%s]", v->name, v->languages, v->identifier);
}

}

/**
 * gdspeak_new:
 *
 * Create a new #gdspeak object.
 *
 */
Gdspeak *
gdspeak_new(void)
{
return g_object_new(GDSPEAK_TYPE, NULL);
}

GSList *
gdspeak_list_voices(Gdspeak *gs)
{
GdspeakPrivate *p=GET_PRIVATE(gs);

return p->voices;
}

gboolean
gdspeak_speak_priority(Gdspeak *gs, guint priority, const gchar *txt)
{
gchar *st;
GdspeakPrivate *p=GET_PRIVATE(gs);

g_return_if_fail(gs, FALSE);
if (!txt)
	return FALSE;

/* We take only valid utf8, bail if it's not */
if (!g_utf8_validate(txt, -1, NULL))
	return FALSE;

if (priority>255)
	priority=255;

switch (priority) {
	case 0:
		speak_stop();
	case 1:
		g_queue_push_head(p->sentences, txt);
	break;
	case 255:
		g_queue_push_tail(p->sentences, txt);
	break;
	default:
		g_queue_push_nth(p->sentences, txt, priority);
	break;
}
st=g_queue_pop_head(p->sentences);
g_return_if_fail(st, FALSE);

return speak_text(st);
}

gboolean
gdspeak_speak(Gdspeak *gs, const gchar *txt)
{
g_return_if_fail(gs, FALSE);

return gdspeak_speak_priority(gs, 100, txt);
}

gboolean
gdspeak_stop(Gdspeak *gs)
{
return speak_stop();
}
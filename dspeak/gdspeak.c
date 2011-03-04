/*
 * This file is part of dspeak
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

typedef struct _Sentence Sentence;
struct _Sentence {
	const gchar *txt;
	guint priority;
	guint32 id;
};

typedef struct _GdspeakPrivate GdspeakPrivate;
struct _GdspeakPrivate {
	GQueue *sentences;
	GSList *voices;
	guint32 id;
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDSPEAK_TYPE, GdspeakPrivate))

static guint signals[LAST_SIGNAL]={ 0 };

/** Sentece tracking helpers **/

static Sentence *
sentence_new(guint32 id, const gchar *txt, guint priority)
{
Sentence *s;

s=g_slice_new0(Sentence);
s->id=id;
s->txt=txt;
if (priority>255)
	priority=255;
s->priority=priority;

return s;
}

static void
sentence_free(Sentence *s)
{
g_return_if_fail(s);

g_free(s->txt);
g_slice_free(Sentence, s);
}

/** The gdspeak object itself **/

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

espeak_Terminate();
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
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
}
}

static void
gdspeak_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
Gdspeak *go=GDSPEAK(object);
gint v;

switch (prop_id) {
	case PROP_PITCH:
		v=espeak_GetParameter(espeakPITCH, 1);
		g_value_set_int(value, v);
	break;
	case PROP_RATE:
		v=espeak_GetParameter(espeakRATE, 1);
		g_value_set_int(value, v);
	break;
	case PROP_RANGE:
		v=espeak_GetParameter(espeakRANGE, 1);
		g_value_set_int(value, v);
	break;
	case PROP_VOLUME:
		v=espeak_GetParameter(espeakVOLUME, 1);
		g_value_set_int(value, v);
	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
}
}

static GObject*
gdspeak_constructor(GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
static GObject *self=NULL;

if (self==NULL) {
	self=G_OBJECT_CLASS(gdspeak_parent_class)->constructor(type, n_construct_params, construct_params);
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

pspec=g_param_spec_uint("pitch", "Pitch", "Speech base pitch, range 0-100.  50=normal", 0, 100, 50, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_PITCH, pspec);

pspec=g_param_spec_uint("range", "Range", "Pitch range, range 0-100. 0-monotone, 50=normal", 0, 100, 50, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_RANGE, pspec);

pspec=g_param_spec_uint("rate", "Rate", "Speech speed, in words per minute", espeakRATE_MINIMUM, espeakRATE_MAXIMUM, espeakRATE_NORMAL, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_RATE, pspec);

pspec=g_param_spec_uint("volume", "Volume", "Speech volume", 0, 100, 50, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_VOLUME, pspec);

speak_init(DEFAULT_LANG);
}

static void 
gdspeak_init(Gdspeak *gs)
{
GdspeakPrivate *p;
const espeak_VOICE **vs;
espeak_VOICE **i;

p=GET_PRIVATE(gs);

p->sentences=g_queue_new();
p->id=1;

vs=espeak_ListVoices(NULL);

for (i=(espeak_VOICE **)vs; *i; i++) {
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

guint32
/**
 * gdspeak_speak_priority:
 *
 * Speak given sentence with the given priority.
 * The smaller the number the higher priority the sentece has. Priority 
 * 0 has the highest priority and will cancel any currently spoken 
 * sentece. The default priority is 100, and lowest is 255.
 * Priority 1 will always go at the top of the queue and 255 and then 
 * end.
 *
 * XXX: Implement queue length limiting. 
 *
 * Returns: Sentence id, larger than 0, zero on error.
 */
gdspeak_speak_priority(Gdspeak *gs, guint priority, const gchar *txt)
{
Sentence *cs, *s;
GdspeakPrivate *p=GET_PRIVATE(gs);

g_return_val_if_fail(gs, FALSE);
if (!txt)
	return FALSE;

/* We take only valid utf8, bail if it's not */
if (!g_utf8_validate(txt, -1, NULL))
	return FALSE;

if (p->id==0)
	p->id=1;
s=sentence_new(p->id++, txt, priority);

switch (priority) {
	case 0:
		gdspeak_stop(gs);
	case 1:
		g_queue_push_head(p->sentences, s);
	break;
	case 255:
		g_queue_push_tail(p->sentences, s);
	break;
	default:
		g_queue_push_nth(p->sentences, s, s->priority);
	break;
}
cs=g_queue_pop_head(p->sentences);
g_return_val_if_fail(cs, FALSE);

if (speak_text(cs->txt))
	return cs->id;
return 0;
}

/**
 * gdspeak_speak:
 *
 * Speak the given text, with default priority. Does not return a 
 * sentence tracking id, just TRUE or FALSE. Mainly for easy speech
 * output when sentence tracking is not required.
 *
 * Returns: TRUE if sentence was succesfully queued, FALSE otherwise.
 */
gboolean
gdspeak_speak(Gdspeak *gs, const gchar *txt)
{
g_return_val_if_fail(gs, FALSE);

return gdspeak_speak_priority(gs, 100, txt)>0 ? TRUE : FALSE;
}

void
gdspeak_clear(Gdspeak *gs)
{
g_return_if_fail(gs);


}

/**
 * gdspeak_stop:
 *
 *
 */
gboolean
gdspeak_stop(Gdspeak *gs)
{
g_return_val_if_fail(gs, FALSE);

return espeak_Cancel()==EE_OK ? TRUE : FALSE;
}

/**
 * gdspeak_speaking:
 *
 *
 */
gboolean
gdspeak_speaking(Gdspeak *gs)
{
g_return_val_if_fail(gs, FALSE);

return espeak_IsPlaying()==1 ? TRUE : FALSE;
}

/**
 * gdspeak_set_voice:
 *
 * Set the voice to use.
 *
 * Returns: TRUE if voice was set, FALSE on error or if voice was not found.
 */
gboolean
gdspeak_set_voice(Gdspeak *gs, const gchar *voice)
{
g_return_val_if_fail(gs, FALSE);

return espeak_SetVoiceByName(voice)==EE_OK ? TRUE : FALSE;
}

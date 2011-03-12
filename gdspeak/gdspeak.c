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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <espeak/speak_lib.h>
#include <string.h>

#ifdef WITH_GST
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappbuffer.h>
#include <gst/app/gstappsink.h>

#ifndef AUDIO_SINK
#define AUDIO_SINK "alsasink"
#endif

#endif

#include "gdspeak.h"

#include "gdspeak-marshal.h"
#include "dspeak-server-glue.h"

#define DEFAULT_VOICE "en"

#define PITCH_MIN (0)
#define PITCH_MAX (100)
#define RANGE_MIN (0)
#define RANGE_MAX (100)
#define VOL_MIN (0)
#define VOL_MAX (100)

G_DEFINE_TYPE(Gdspeak, gdspeak, G_TYPE_OBJECT);

/* Signal IDs */
enum {
	SIGNAL_0,
	SIGNAL_START,
	SIGNAL_SENTENCE_START,
	SIGNAL_WORD,
	SIGNAL_MARK,
	SIGNAL_PLAY,
	SIGNAL_PHONEME,
	SIGNAL_SENTENCE_END,
	SIGNAL_END,
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

#ifdef WITH_GST
typedef struct _GstEspeak GstEspeak;
struct _GstEspeak {
	GstCaps	*srccaps;
	GstElement *pipeline;
	GstElement *ac;
	GstElement *src;
	GstElement *queue;
	GstElement *sink;
	GstBus *bus;
	gboolean eos;
	gboolean speaking;
	gshort *buffer;
	gint size;
	gchar *text;
};

/***
 * For old gstreamer in Diablo 
 */
#ifndef GST_MESSAGE_SRC_NAME
#define GST_MESSAGE_SRC_NAME(message)   (GST_MESSAGE_SRC(message) ? \
     GST_OBJECT_NAME (GST_MESSAGE_SRC(message)) : "(NULL)")
#endif
#ifndef GST_CHECK_VERSION
#define	GST_CHECK_VERSION(major,minor,micro)	\
    (GST_VERSION_MAJOR > (major) || \
    (GST_VERSION_MAJOR == (major) && GST_VERSION_MINOR > (minor)) || \
    (GST_VERSION_MAJOR == (major) && GST_VERSION_MINOR == (minor) && \
     GST_VERSION_MICRO >= (micro)))
#endif

#endif

typedef struct _SProperties SProperties;
struct _SProperties {
	const gchar *lang;
	gint pitch;
	gint speed;
	gint rate;
	gint range;
	gint volume;
};

typedef struct _Sentence Sentence;
struct _Sentence {
	guint32 id;
	gchar *txt;
	guint priority;
	gchar *lc;
	SProperties sp;
	gpointer data;
};

typedef struct _GdspeakPrivate GdspeakPrivate;
struct _GdspeakPrivate {
	GQueue *sentences;
	GHashTable *voices;
	Sentence *cs;
#ifdef WITH_GST
	GstEspeak ge;
#endif
	gint srate;
	SProperties sp;
	guint32 id;
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDSPEAK_TYPE, GdspeakPrivate))

static guint signals[LAST_SIGNAL]={ 0 };

static gboolean gdspeak_speak_next_sentence(GdspeakPrivate *p);

/** Sentece tracking helpers **/

static Sentence *
sentence_new(guint32 id, const gchar *txt, guint priority)
{
Sentence *s;

s=g_slice_new0(Sentence);
s->id=id;
s->txt=g_strdup(txt);
if (priority>255)
	priority=255;
s->priority=priority;
s->data=NULL;
return s;
}

static void
sentence_free(Sentence *s)
{
g_return_if_fail(s);

g_free(s->txt);
g_free((gpointer)s->sp.lang);
g_slice_free(Sentence, s);
}

/** Helpers **/
static void
espeak_set_properties(SProperties *sp)
{
if (sp->lang)
	espeak_SetVoiceByName(sp->lang);
if (sp->pitch>0)
	espeak_SetParameter(espeakPITCH, sp->pitch, 0);
if (sp->rate>0)
	espeak_SetParameter(espeakRATE, sp->rate, 0);
if (sp->range>0)
	espeak_SetParameter(espeakRANGE, sp->range, 0);
if (sp->volume>-1)
	espeak_SetParameter(espeakVOLUME, sp->volume, 0);
}

static gboolean
speak_sentence(Sentence *s)
{
const gchar *text;
size_t tlen;
espeak_ERROR ee;

text=s->txt;
tlen=strlen(text);

espeak_set_properties(&s->sp);

g_debug("Speak: [%s] (%zu)", text, tlen);
ee=espeak_Synth(text, tlen+1, 0, POS_CHARACTER, 0, espeakCHARS_UTF8 | espeakENDPAUSE, &s->id, s->data);
switch (ee) {
	case EE_OK:
		return TRUE;
	case EE_BUFFER_FULL:
		g_warning("ESBUFFUL");
		return FALSE;
	case EE_INTERNAL_ERROR:
		g_warning("ESINTERR");
		return FALSE;
	default:
		g_warning("????????");
		return FALSE;
}
g_warning("!?!?!?!");
return FALSE;
}

/**
 * Gstreamer output mode
 */
#ifdef WITH_GST

static gboolean 
gst_espeak_stop_cb(gpointer data)
{
GdspeakPrivate *p=data;

g_assert(p);

espeak_Cancel();
gst_element_set_state(p->ge.pipeline, GST_STATE_NULL);
return FALSE;
}

static gboolean
gst_espeak_bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
gchar *debug;
GstState newstate;
GstState oldstate;
GstState pending;
GError *err=NULL;
GdspeakPrivate *p=data;

g_assert(p);

switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_EOS:
		g_debug("EOS from %s", GST_MESSAGE_SRC_NAME(msg));
		p->ge.eos=TRUE;
		g_timeout_add(500, (GSourceFunc)gst_espeak_stop_cb, p);
	break;
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &err, &debug);
		g_debug("Error: %s", err->message);
		g_free(debug);
		g_error_free(err);

		gst_espeak_stop_cb(p);
	break;
	case GST_MESSAGE_STATE_CHANGED:
		p->ge.eos=FALSE;
		gst_message_parse_state_changed(msg, &oldstate, &newstate, &pending);
		g_debug("GST: %s state changed (o=%d->n=%d => p=%d)", GST_MESSAGE_SRC_NAME(msg), oldstate, newstate, pending);

		/* We are only interested in pipeline messages */
		if (GST_MESSAGE_SRC(msg)!=GST_OBJECT(p->ge.pipeline))
			return TRUE;

		/* Pipe is going to pause, we can start pushing buffers */
		if (pending==GST_STATE_PAUSED) {
			g_debug("Preparing next sentence");
			g_idle_add((GSourceFunc)gdspeak_speak_next_sentence, p);
		}
	break;
	default:
		g_debug("GST: From %s -> %s", GST_MESSAGE_SRC_NAME(msg), gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
	break;
	}
return TRUE;
}

static gboolean
gst_espeak_create_pipeline(GdspeakPrivate *p)
{
p->ge.eos=TRUE;
p->ge.pipeline=gst_pipeline_new("pipeline");
g_return_val_if_fail(p->ge.pipeline, FALSE);

p->ge.src=gst_element_factory_make("appsrc", "source");
g_return_val_if_fail(p->ge.src, FALSE);

#if GST_CHECK_VERSION(0,10,22)
gst_app_src_set_size(GST_APP_SRC(p->ge.src), -1);
gst_app_src_set_stream_type(GST_APP_SRC(p->ge.src), GST_APP_STREAM_TYPE_STREAM);
/* g_object_set(GST_APP_SRC(p->ge.src), "block", TRUE, NULL); */
g_object_set(GST_APP_SRC(p->ge.src), "is-live", TRUE, NULL);
#endif

p->ge.srccaps=gst_caps_new_simple("audio/x-raw-int",
			"depth", G_TYPE_INT, 16,
			"width", G_TYPE_INT,  16,
			"signed", G_TYPE_BOOLEAN, TRUE, 
			"rate", G_TYPE_INT, p->srate,
			"channels", G_TYPE_INT, 1,
			"endianness", G_TYPE_INT, G_LITTLE_ENDIAN, /* XXX: Check what espeak does on big endian */
			NULL);
g_return_val_if_fail(p->ge.srccaps, FALSE);

p->ge.ac=gst_element_factory_make("audioconvert", "ac");
g_return_val_if_fail(p->ge.ac, FALSE);

p->ge.queue=gst_element_factory_make("queue", "queue");
g_return_val_if_fail(p->ge.queue, FALSE);

p->ge.sink=gst_element_factory_make(AUDIO_SINK, "sink");
g_return_val_if_fail(p->ge.sink, FALSE);

gst_bin_add_many(GST_BIN(p->ge.pipeline), p->ge.src, p->ge.ac, p->ge.queue, p->ge.sink, NULL);

if (!gst_element_link_filtered(p->ge.src, p->ge.ac, p->ge.srccaps)) {
	g_warning("Failed to link src->ac with caps.");
	return FALSE;
}
gst_caps_unref(p->ge.srccaps);

if (!gst_element_link(p->ge.ac, p->ge.queue)) {
	g_warning("Failed to link ac->queue.");
	return FALSE;
}

if (!gst_element_link(p->ge.queue, p->ge.sink)) {
	g_warning("Failed to link queue->sink.");
	return FALSE;
}

p->ge.bus=gst_pipeline_get_bus(GST_PIPELINE(p->ge.pipeline));
g_return_val_if_fail(p->ge.bus, FALSE);

gst_bus_add_watch(p->ge.bus, gst_espeak_bus_call, p);

return TRUE;
}

static void
gst_espeak_destroy_pipeline(GdspeakPrivate *p)
{
gst_element_set_state(p->ge.pipeline, GST_STATE_NULL);
gst_object_unref(p->ge.pipeline);
}

static void
gst_espeak_buffer_free(void *p)
{
g_free(p);
}

static gboolean
gst_espeak_start_play(gpointer data)
{
GdspeakPrivate *p=data;

g_assert(p);

g_debug("PLAYING");
if (gst_element_set_state(p->ge.pipeline, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE)
	g_warning("Failed to play pipeline");
return FALSE;
}

static int 
gst_espeak_cb(GdspeakPrivate *p, short *wav, int numsamples)
{
GstBuffer *buf;
gchar *data;

g_debug("W: %d (%d)", wav==NULL ? 0 : 1, numsamples);

if (wav==NULL) {
#if GST_CHECK_VERSION(0,10,22)
	if (gst_app_src_end_of_stream(GST_APP_SRC(p->ge.src))!=GST_FLOW_OK)
		g_warning("Failed to push EOS");
#else
	gst_app_src_end_of_stream(GST_APP_SRC(p->ge.src));
#endif
	p->ge.eos=TRUE;
	return 0;
}
if (numsamples>0) {
	g_debug("Adding speach buffer %d", numsamples);

	numsamples=numsamples*2;
	data=g_memdup(wav, numsamples);
	buf=gst_app_buffer_new(data, numsamples, gst_espeak_buffer_free, data);
	gst_buffer_set_caps(buf, p->ge.srccaps);
#if GST_CHECK_VERSION(0,10,22)
	if (gst_app_src_push_buffer(GST_APP_SRC(p->ge.src), buf)!=GST_FLOW_OK)
		g_warning("Failed to push buffer");
#else
	gst_app_src_push_buffer(GST_APP_SRC(p->ge.src), buf);
#endif
}
return 0;
}
#endif

/**
 * speak_synth_cb:
 *
 * Callback to handle events from espeak. Will emit signals with the event information.
 *
 */
static int 
speak_synth_cb(short *wav, int numsamples, espeak_EVENT *events)
{
Gdspeak *ds;
GdspeakPrivate *p;
espeak_EVENT *e;

g_debug("CB:");

ds=GDSPEAK(events->user_data);
g_return_val_if_fail(IS_GDSPEAK(ds), 0);
p=GET_PRIVATE(ds);

#ifdef WITH_GST
gst_espeak_cb(p, wav, numsamples);
#endif

for (e=events;e->type!=espeakEVENT_LIST_TERMINATED;e++) {
	g_debug("SCB: id=%d type=%d pos=%d len=%d apos=%d", e->unique_identifier, e->type, e->text_position, e->length, e->audio_position);
	switch (e->type) {
	case espeakEVENT_SENTENCE:
		g_debug("S: %d", e->id.number);
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_SENTENCE_START], 0, e->unique_identifier, e->id.number, NULL);
	break;
	case espeakEVENT_WORD:
		g_debug("W: %d", e->id.number);
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_WORD], 0, e->unique_identifier, e->text_position, e->id.number, NULL);
	break;
	case espeakEVENT_MARK:
		g_debug("M: %s", e->id.name);
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_MARK], 0, e->unique_identifier, e->id.name, NULL);
	break;
	case espeakEVENT_PLAY:
		g_debug("P: %s", e->id.name);
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_PLAY], 0, e->unique_identifier, e->id.name, NULL);
	break;
	case espeakEVENT_END:
		g_debug("E:");
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_SENTENCE_END], 0, e->unique_identifier, NULL);
	break;
	case espeakEVENT_PHONEME:
		g_debug("PH: %d", e->id.number);
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_PHONEME], 0, e->id.number, NULL);
	break;
	case espeakEVENT_MSG_TERMINATED:
		g_debug("MT:");
		g_signal_emit(G_OBJECT(ds), signals[SIGNAL_END], 0, e->unique_identifier, NULL);
		sentence_free(p->cs);
		p->cs=NULL;
		if (g_queue_is_empty(p->sentences)==FALSE)
			g_idle_add((GSourceFunc)gdspeak_speak_next_sentence, p);
	break;
	case espeakEVENT_LIST_TERMINATED:
		g_debug("LT:");
		return 0;
	break;
	case espeakEVENT_SAMPLERATE:
	default:
		return 0;
	}
}
return 0;
}

static int
speak_uri_cb(int type, const char *uri, const char *base)
{
g_debug("UCB: t=%d uri=%s base=%s", type, uri, base);
return 0;
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
Gdspeak *gs=GDSPEAK(object);
GdspeakPrivate *p;

g_return_if_fail(gs);
G_OBJECT_CLASS(gdspeak_parent_class)->finalize(object);

p=GET_PRIVATE(gs);
g_hash_table_destroy(p->voices);
#ifdef WITH_GST
gst_espeak_destroy_pipeline(p);
#endif
espeak_Terminate();
}

static void
gdspeak_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
Gdspeak *gs=GDSPEAK(object);
GdspeakPrivate *p;
gint v;

p=GET_PRIVATE(gs);

v=g_value_get_int(value);
switch (prop_id) {
	case PROP_PITCH:
		p->sp.pitch=v;
		espeak_SetParameter(espeakPITCH, v, 0);
	break;
	case PROP_RATE:
		p->sp.rate=v;
		espeak_SetParameter(espeakRATE, v, 0);
	break;
	case PROP_RANGE:
		p->sp.range=v;
		espeak_SetParameter(espeakRANGE, v, 0);
	break;
	case PROP_VOLUME:
		p->sp.volume=v;
		espeak_SetParameter(espeakVOLUME, v, 0);
	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	break;
}
}

static void
gdspeak_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
Gdspeak *gs=GDSPEAK(object);
GdspeakPrivate *p;

p=GET_PRIVATE(gs);

switch (prop_id) {
	case PROP_PITCH:
		p->sp.pitch=espeak_GetParameter(espeakPITCH, 1);
		g_value_set_int(value, p->sp.pitch);
	break;
	case PROP_RATE:
		p->sp.rate=espeak_GetParameter(espeakRATE, 1);
		g_value_set_int(value, p->sp.rate);
	break;
	case PROP_RANGE:
		p->sp.range=espeak_GetParameter(espeakRANGE, 1);
		g_value_set_int(value, p->sp.range);
	break;
	case PROP_VOLUME:
		p->sp.volume=espeak_GetParameter(espeakVOLUME, 1);
		g_value_set_int(value, p->sp.volume);
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

/**
 * Properties **/
pspec=g_param_spec_uint("pitch", "Pitch", "Speech base pitch, range 0-100.  50=normal", PITCH_MIN, PITCH_MAX, 50, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_PITCH, pspec);

pspec=g_param_spec_uint("range", "Range", "Pitch range, range 0-100. 0-monotone, 50=normal", RANGE_MIN, RANGE_MAX, 50, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_RANGE, pspec);

pspec=g_param_spec_uint("rate", "Rate", "Speech speed, in words per minute", espeakRATE_MINIMUM, espeakRATE_MAXIMUM, espeakRATE_NORMAL, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_RATE, pspec);

pspec=g_param_spec_uint("volume", "Volume", "Speech volume", VOL_MIN, VOL_MAX, 50, G_PARAM_READWRITE);
g_object_class_install_property(object_class, PROP_VOLUME, pspec);

/**
 * Signals **/
signals[SIGNAL_START]=
	g_signal_new("speak-start", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 0);
signals[SIGNAL_SENTENCE_START]=
	g_signal_new("sentence-start", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, _gdspeak_VOID__INT_INT, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
signals[SIGNAL_WORD]=
	g_signal_new("word", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, _gdspeak_VOID__INT_INT, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
signals[SIGNAL_MARK]=
	g_signal_new("mark", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, _gdspeak_VOID__INT_STRING, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_STRING);
signals[SIGNAL_PLAY]=
	g_signal_new("play", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, _gdspeak_VOID__INT_STRING, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_STRING);
signals[SIGNAL_PHONEME]=
	g_signal_new("phoneme", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
signals[SIGNAL_SENTENCE_END]=
	g_signal_new("sentence-end", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
signals[SIGNAL_END]=
	g_signal_new("speak-end", G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

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

#ifdef WITH_GST
p->srate=espeak_Initialize(AUDIO_OUTPUT_RETRIEVAL, 200, NULL, 0);
#else
p->srate=espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 250, NULL, 0);
#endif
if (p->srate==-1) {
	g_warning("Failed to initialize espeak");
	return;
}
espeak_SetSynthCallback(speak_synth_cb);
espeak_SetUriCallback(speak_uri_cb);

p->sp.pitch=50;
p->sp.range=50;
p->sp.rate=espeakRATE_NORMAL;
p->sp.volume=50;
p->sp.lang=g_strdup(DEFAULT_VOICE);

espeak_set_properties(&p->sp);

vs=espeak_ListVoices(NULL);
p->voices=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

for (i=(espeak_VOICE **)vs; *i; i++) {
	espeak_VOICE *v=*i;
	g_debug("V: [%s] (%s) [%s]", v->name, v->languages, v->identifier);
	g_hash_table_insert(p->voices, g_strdup(v->identifier), g_strdup(v->name));
}

#ifdef WITH_GST
gst_espeak_create_pipeline(p);
if (gst_element_set_state(p->ge.pipeline, GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
	g_warning("Failed to prepare pipeline to paused.");
}
#endif

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

GHashTable *
gdspeak_list_voices(Gdspeak *gs)
{
GdspeakPrivate *p=GET_PRIVATE(gs);

return p->voices;
}

static gboolean
gdspeak_push_sentence(GdspeakPrivate *p, Sentence *s)
{
switch (s->priority) {
	case 0:
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
return TRUE;
}

static gboolean
gdspeak_speak_next_sentence(GdspeakPrivate *p)
{
g_debug("NS");

#ifdef WITH_GST
if (p->ge.eos==TRUE) {
	g_idle_add_full(G_PRIORITY_HIGH_IDLE, (GSourceFunc)gst_espeak_start_play, p, NULL);
	return TRUE;
}
#endif

p->cs=g_queue_pop_head(p->sentences);
if (!p->cs) {
	g_debug("Queue is empty");
	return FALSE;
}
if (p->cs->priority==0) {
	g_debug("P0");
	espeak_Cancel();
}
g_debug("SP");
if (speak_sentence(p->cs)==FALSE) {
	g_warning("Failed to speak");
	sentence_free(p->cs);
	p->cs=NULL;
}
return FALSE;
}

/**
 * gdspeak_speak_full:
 *
 * Speak given sentence with the given priority and speech settings.
 *
 * Set settings to -1 to use current for pitch, range, rate and volume.
 *
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
guint32
gdspeak_speak_full(Gdspeak *gs, const gchar *txt, const gchar *lang, guint priority, gint pitch, gint range, gint rate, gint volume)
{
Sentence *s;
GdspeakPrivate *p;

g_return_val_if_fail(gs, 0);
if (!txt)
	return 0;

/* We take only valid utf8, bail if it's not */
if (!g_utf8_validate(txt, -1, NULL))
	return 0;

p=GET_PRIVATE(gs);
if (p->id==0)
	p->id=1;
s=sentence_new(p->id++, txt, priority);
if (lang && g_hash_table_lookup(p->voices, lang))
	s->sp.lang=lang;
else
	s->sp.lang=NULL;
s->sp.pitch=pitch>-1 ? CLAMP(pitch, PITCH_MIN, PITCH_MAX) : -1;
s->sp.range=range>-1 ? CLAMP(pitch, RANGE_MIN, RANGE_MAX) : -1;
s->sp.rate=rate>-1 ? CLAMP(pitch, espeakRATE_MINIMUM, espeakRATE_MAXIMUM) : -1;
s->sp.volume=volume>-1 ? CLAMP(pitch, VOL_MIN, VOL_MAX) : -1;
s->data=gs;

gdspeak_push_sentence(p, s);
if (espeak_IsPlaying()!=1)
	g_idle_add((GSourceFunc)gdspeak_speak_next_sentence, p);
return s->id;
}

/**
 * gdspeak_speak_priority:
 *
 * Speak given sentence with the given priority and the current speech settings.
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
guint32
gdspeak_speak_priority(Gdspeak *gs, guint priority, const gchar *txt)
{
return gdspeak_speak_full(gs, txt, NULL, priority, -1, -1, -1, -1);
}

/**
 * gdspeak_speak:
 *
 * Speak the given text, with default priority and the current speech 
 * settings. Does not return a  sentence tracking id, just TRUE or FALSE.
 *
 * Mainly for easy speech output when sentence tracking is not required.
 *
 * Returns: TRUE if sentence was succesfully queued, FALSE otherwise.
 */
gboolean
gdspeak_speak(Gdspeak *gs, const gchar *txt)
{
g_return_val_if_fail(gs, FALSE);

return gdspeak_speak_priority(gs, 100, txt)>0 ? TRUE : FALSE;
}

/**
 * gdspeak_clear:
 *
 * Empty the sentence queue.
 */
void
gdspeak_clear(Gdspeak *gs)
{
GdspeakPrivate *p;

g_return_if_fail(gs);

p=GET_PRIVATE(gs);
g_return_if_fail(p->sentences);

g_queue_foreach(p->sentences, (GFunc)sentence_free, NULL);
}

/**
 * gdspeak_stop:
 *
 *
 */
gboolean
gdspeak_stop(Gdspeak *gs, gboolean clear)
{
g_return_val_if_fail(gs, FALSE);
if (clear)
	gdspeak_clear(gs);
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

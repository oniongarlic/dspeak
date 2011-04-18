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

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gdspeak.h"
#include "gdbus-speak.h"
#include "gdspeak-server-glue.h"

G_DEFINE_TYPE(Gdbusspeak, gdbusspeak, GDSPEAK_TYPE);

#define TIMEOUT_SECS (60)

typedef struct _GdbusspeakPrivate GdbusspeakPrivate;
struct _GdbusspeakPrivate {
	gint ts_src;
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDBUSSPEAK_TYPE, GdbusspeakPrivate))

static gboolean
gdbusspeak_timeout_exit(gpointer data)
{
Gdbusspeak *gs=(Gdbusspeak *)data;
guint et;
time_t ct;

if (gdspeak_speaking(GDSPEAK(gs)))
	return TRUE;
ct=time(NULL);

g_object_get(gs, "et", &et, NULL);

g_debug("%d - %d = %d", ct, et, ct-et);
if (ct-et<60)
	return TRUE;

g_debug("Timeout, byebye");
g_object_unref(gs);
exit(0);
return FALSE;
}

static void
gdbusspeak_remove_timeout(Gdbusspeak *gs)
{
GdbusspeakPrivate *p;

p=GET_PRIVATE(gs);

if (p->ts_src==0)
	return;
    
g_source_remove(p->ts_src);
p->ts_src=0;
}

static void
gdbusspeak_add_timeout(Gdbusspeak *gs)
{
GdbusspeakPrivate *p;

p=GET_PRIVATE(gs);

gdbusspeak_remove_timeout(gs);
#if GLIB_CHECK_VERSION(2,14,0)
p->ts_src=g_timeout_add_seconds(TIMEOUT_SECS, gdbusspeak_timeout_exit, gs);
#else
p->ts_src=g_timeout_add(TIMEOUT_SECS*1000, gdbusspeak_timeout_exit, gs);
#endif
}


/** The gdbusspeak object itself **/

static void
gdbusspeak_dispose(GObject *object)
{
Gdbusspeak *gs=GDBUSSPEAK(object);

gdbusspeak_remove_timeout(gs);

G_OBJECT_CLASS(gdbusspeak_parent_class)->dispose(object);
}

static void
gdbusspeak_finalize(GObject *object)
{
Gdbusspeak *gs=GDBUSSPEAK(object);

g_return_if_fail(gs);
G_OBJECT_CLASS(gdbusspeak_parent_class)->finalize(object);
}

static void
gdbusspeak_class_init(GdbusspeakClass *klass)
{
GObjectClass *object_class=G_OBJECT_CLASS(klass);

object_class->dispose=gdbusspeak_dispose;
object_class->finalize=gdbusspeak_finalize;

dbus_g_object_type_install_info(gdbusspeak_get_type(), &dbus_glib_gdspeak_object_info);

g_type_class_add_private(object_class, sizeof(GdbusspeakPrivate));
}

static void 
gdbusspeak_init(Gdbusspeak *gs)
{
gdbusspeak_add_timeout(gs);
}

/**
 * gdbusspeak_new:
 *
 * Create a new #gdbusspeak object ready to be put on D-Bus.
 *
 */
Gdbusspeak *
gdbusspeak_new(void)
{
return g_object_new(GDBUSSPEAK_TYPE, NULL);
}


/*
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
#include <string.h>

#include <glib.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#ifdef WITH_GST
#include <gst/gst.h>
#endif

#include "gdspeak.h"

/**
 * get_default_language:
 *
 * Get default language code to use. Tries to get a language from the current locale.
 *
 */
static
gchar *get_default_language(GHashTable *voices)
{
gchar *lcm;

g_assert(voices);

lcm=getenv("LC_MESSAGES");
if (!lcm)
	lcm=getenv("LANG");
if (!lcm)
	goto def;
if (strlen(lcm)<2)
	goto def;
lcm=g_strndup(lcm, 2);

if (g_hash_table_lookup_extended(voices, lcm, NULL, NULL)==TRUE)
	return lcm;

g_free(lcm);

def:;
return g_strdup("en");
}

gint
main(gint argc, gchar **argv)
{
DBusGConnection *conn;
DBusGProxy *proxy;
GError *error=NULL;
GMainLoop *mainloop;
Gdspeak *ds;
gchar *lang;
guint32 rname;

g_type_init();

#ifdef WITH_GST
gst_init(&argc, &argv);
#endif

mainloop=g_main_loop_new(NULL, FALSE);

conn=dbus_g_bus_get(DBUS_BUS_SESSION, &error);
if (!conn) {
	g_error("Error getting bus: %s", error->message);
	return 1;
}

proxy=dbus_g_proxy_new_for_name(conn, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

if (!org_freedesktop_DBus_request_name(proxy, GDSPEAK_NAME_DBUS, 0, &rname, &error)) {
	g_error ("Error registering D-Bus service %s: %s", GDSPEAK_NAME_DBUS, error->message);
	return 1;
}

if (rname != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	return 1;

ds=gdspeak_new();

lang=get_default_language(gdspeak_list_voices(ds));

g_debug("Default language: %s", lang);

gdspeak_set_voice(ds, lang);

dbus_g_connection_register_g_object(conn, GDSPEAK_PATH_DBUS, G_OBJECT(ds));

g_main_loop_run(mainloop);

g_free(lang);
g_object_unref(ds);

return 0;
}

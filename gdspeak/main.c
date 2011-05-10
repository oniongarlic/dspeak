/*
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <gconf/gconf-client.h>

#ifdef WITH_GST
#include <gst/gst.h>
#endif

#include "gdspeak.h"
#include "gdbus-speak.h"

#define GDS_GCONF_PATH "/apps/gdspeak"
#define GDS_GCONF_LANGUAGE GDS_GCONF_PATH  "/language"
#define GDS_GCONF_PITCH GDS_GCONF_PATH  "/pitch"
#define GDS_GCONF_RATE GDS_GCONF_PATH  "/rate"
#define GDS_GCONF_RANGE GDS_GCONF_PATH  "/range"

GConfClient *client;
DBusGConnection *conn;
DBusGProxy *proxy;
GMainLoop *mainloop;
Gdbusspeak *ds;
GHashTable *voices;
gchar *lang;

static gchar *
get_env_language()
{
gchar *lcm;

lcm=getenv("LC_MESSAGES");
if (!lcm)
	lcm=getenv("LANG");
if (!lcm)
	return NULL;
if (strlen(lcm)<2)
	return NULL;
lcm=g_strndup(lcm, 2);
return lcm;
}

/**
 * get_default_language:
 *
 * Get default language code to use. Tries to get a language from the current locale.
 *
 */
static gchar *
get_default_language(void)
{
GError *error=NULL;
gchar *lcm=NULL;

if (client)
	lcm=gconf_client_get_string(client, GDS_GCONF_LANGUAGE, &error);
if (error) {
	g_error("Failed to get default language from gconf: %s", error->message);
	g_error_free(error);
}
if (lcm==NULL) {
	lcm=get_env_language();
}
if (lcm==NULL)
	goto def;

if (g_hash_table_lookup_extended(voices, lcm, NULL, NULL)==TRUE)
	return lcm;

g_free(lcm);

def:;
return g_strdup("en");
}

static void
settings_changed_cb(GConfClient *client, guint cnxn_id, GConfEntry *entry, gpointer user_data)
{
gchar *lcm=NULL;

g_debug("Settings updated, reloading");

lcm=gconf_client_get_string(client, GDS_GCONF_LANGUAGE, NULL);
if (!lcm)
	return;
if (g_hash_table_lookup_extended(voices, lcm, NULL, NULL)==FALSE) {
	g_free(lcm);
	return;
}

g_debug("New default language: %s", lcm);
gdspeak_set_default_voice(GDSPEAK(ds), lcm);
g_free(lcm);
} 

gint
main(gint argc, gchar **argv)
{
GError *error=NULL;
guint32 rname, stmp;

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

if (!org_freedesktop_DBus_request_name(proxy, GDBUSSPEAK_NAME_DBUS, 0, &rname, &error)) {
	g_error("Error registering D-Bus service %s: %s", GDBUSSPEAK_NAME_DBUS, error->message);
	return 1;
}

if (rname != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	return 1;

ds=gdbusspeak_new();

client=gconf_client_get_default();
g_assert(client);

voices=gdspeak_list_voices(GDSPEAK(ds));

lang=get_default_language();
g_debug("Default language: %s", lang);
gdspeak_set_voice(GDSPEAK(ds), lang);

if (gconf_client_set_string(client, GDS_GCONF_LANGUAGE, lang, NULL)==FALSE)
	g_warning("Failed to store current language");

gconf_client_suggest_sync(client, NULL);

g_free(lang);
lang=NULL;

stmp=gconf_client_get_int(client, GDS_GCONF_PITCH, NULL);
if (stmp>0)
	g_object_set(ds, "pitch", stmp, NULL);
stmp=gconf_client_get_int(client, GDS_GCONF_RANGE, NULL);
if (stmp>0)
	g_object_set(ds, "range", stmp, NULL);
stmp=gconf_client_get_int(client, GDS_GCONF_RATE, NULL);
if (stmp>0)
	g_object_set(ds, "rate", stmp, NULL);


gconf_client_add_dir(client, GDS_GCONF_PATH, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
gconf_client_notify_add(client, GDS_GCONF_PATH, settings_changed_cb, ds, NULL, NULL);

dbus_g_connection_register_g_object(conn, GDBUSSPEAK_PATH_DBUS, G_OBJECT(ds));

g_main_loop_run(mainloop);

g_object_unref(ds);
g_object_unref(client);

#ifdef WITH_GST
gst_deinit();
#endif

return 0;
}

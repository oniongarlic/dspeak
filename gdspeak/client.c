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
#include <config.h>
#endif

#include <glib.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "gdspeak.h"

#include "dspeak-client-glue.h"

gint
main(gint argc, gchar **argv)
{
DBusGConnection *conn;
DBusGProxy *proxy;
GError *error=NULL;
gboolean r,cr;

g_type_init();

conn=dbus_g_bus_get(DBUS_BUS_SESSION, &error);
if (!conn) {
	g_error("Error getting bus: %s", error->message);
	return 1;
}

proxy=dbus_g_proxy_new_for_name(conn, GDSPEAK_NAME_DBUS, GDSPEAK_PATH_DBUS, GDSPEAK_INTERFACE_DBUS);

cr=org_tal_gdspeak_speak(proxy, argc > 1 ? argv[1] : "I need one argument", &r, &error);
if (!cr) {
	g_error("Failed: %s", error->message);
	return 1;
}

g_object_unref(proxy);

return 0;
}
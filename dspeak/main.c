#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "gdspeak.h"

#define DSPEAK_NAME "org.tal.gdspeak"
#define DSPEAK_PATH "/org/tal/gdspeak"

gint
main(gint argc, gchar **argv)
{
DBusGConnection *conn;
DBusGProxy *proxy;
GError *error=NULL;
GMainLoop *mainloop;
Gdspeak *ds;
guint32 rname;

g_type_init();

mainloop=g_main_loop_new(NULL, FALSE);

conn=dbus_g_bus_get(DBUS_BUS_SESSION, &error);
if (!conn) {
	g_error("Error getting bus: %s", error->message);
	return 1;
}

proxy=dbus_g_proxy_new_for_name(conn, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

if (!org_freedesktop_DBus_request_name(proxy, DSPEAK_NAME, 0, &rname, &error)) {
	g_error ("Error registering D-Bus service %s: %s", DSPEAK_NAME, error->message);
	return 1;
}

if (rname != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	return 1;

ds=gdspeak_new();
dbus_g_connection_register_g_object(conn, DSPEAK_PATH, G_OBJECT(ds));

g_main_loop_run(mainloop);
return 0;
}

INCLUDES = -DDATADIR=\""$(datadir)"\" -DDBUS_API_SUBJECT_TO_CHANGE

lib_LTLIBRARIES = libgdspeak.la

libgdspeak_la_SOURCES = gdspeak.c gdspeak.h gdspeak-marshal.c gdspeak-marshal.h
libgdspeak_la_CFLAGS = $(ESPEAK_CFLAGS) $(GLIB_CFLAGS) $(GSTREAMER_CFLAGS) $(defines)
libgdspeak_la_LIBADD = $(ESPEAK_LIBS) $(GLIB_LIBS) $(GSTREAMER_LIBS)
libgdspeak_la_LDFLAGS = -lm -no-undefined -version-info 0:0:0

libgdspeak_includedir=$(includedir)/gdspeak/
libgdspeak_include_HEADERS = gdspeak-client-glue.h

libexec_PROGRAMS=gdspeak
bin_PROGRAMS=gdspeak-client

gdspeak_CFLAGS = $(GLIB_CFLAGS) $(ESPEAK_CFLAGS) $(GSTREAMER_CFLAGS)
gdspeak_LDADD = $(GLIB_LIBS) $(ESPEAK_LIBS) $(GSTREAMER_LIBS) libgdspeak.la
gdspeak_SOURCES=main.c dspeak-server-glue.h

gdspeak_client_CFLAGS = $(GLIB_CFLAGS)
gdspeak_client_LDADD = $(GLIB_LIBS)
gdspeak_client_SOURCES=client.c dspeak-client-glue.h

BUILT_SOURCES=gdspeak-server-glue.h gdspeak-client-glue.h gdspeak-marshal.c gdspeak-marshal.h

EXTRA_DIST=gdspeak-marshal.list

gdspeak-server-glue.h: gdspeak-interface.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=gdspeak gdspeak-interface.xml > gdspeak-server-glue.h

gdspeak-client-glue.h: gdspeak-interface.xml
	$(DBUS_BINDING_TOOL) --mode=glib-client --prefix=gdspeak gdspeak-interface.xml > gdspeak-client-glue.h

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.tal.gdspeak.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

gdspeak-marshal.c: gdspeak-marshal.list
	glib-genmarshal --prefix _gdspeak --body $< > $@

gdspeak-marshal.h: gdspeak-marshal.list
	glib-genmarshal --prefix _gdspeak --header $< > $@

INCLUDES = -DDATADIR=\""$(datadir)"\" -DDBUS_API_SUBJECT_TO_CHANGE

lib_LTLIBRARIES = libgdspeak.la

libgdspeak_la_SOURCES = gdspeak.c gdspeak.h gdspeak-marshal.c gdspeak-marshal.h
libgdspeak_la_CFLAGS = $(ESPEAK_CFLAGS) $(GLIB_CFLAGS) $(defines)
libgdspeak_la_LIBADD = $(ESPEAK_LIBS) $(GLIB_LIBS)
libgdspeak_la_LDFLAGS = -lm -no-undefined -version-info 0:0:0

libexec_PROGRAMS=dspeak
bin_PROGRAMS=dspeak-client

dspeak_CFLAGS = $(GLIB_CFLAGS) $(ESPEAK_CFLAGS)
dspeak_LDADD = $(GLIB_LIBS) $(ESPEAK_LIBS) libgdspeak.la
dspeak_SOURCES=main.c dspeak-server-glue.h

dspeak_client_CFLAGS = $(GLIB_CFLAGS)
dspeak_client_LDADD = $(GLIB_LIBS)
dspeak_client_SOURCES=client.c dspeak-client-glue.h

BUILT_SOURCES=dspeak-server-glue.h dspeak-client-glue.h gdspeak-marshal.c gdspeak-marshal.h

EXTRA_DIST=gdspeak-marshal.list

dspeak-server-glue.h: dspeak-interface.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=gdspeak dspeak-interface.xml > dspeak-server-glue.h

dspeak-client-glue.h: dspeak-interface.xml
	$(DBUS_BINDING_TOOL) --mode=glib-client --prefix=gdspeak dspeak-interface.xml > dspeak-client-glue.h

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.tal.gdspeak.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

gdspeak-marshal.c: gdspeak-marshal.list
	glib-genmarshal --prefix _gdspeak --body $< > $@

gdspeak-marshal.h: gdspeak-marshal.list
	glib-genmarshal --prefix _gdspeak --header $< > $@


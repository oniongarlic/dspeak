INCLUDES = -DDATADIR=\""$(datadir)"\" -DDBUS_API_SUBJECT_TO_CHANGE -DG_LOG_DOMAIN=\"gdspeak\"

lib_LTLIBRARIES = libgdspeak.la libgdbusspeak.la

libgdspeak_la_SOURCES = gdspeak.c gdspeak.h gdspeak-marshal.c gdspeak-marshal.h
libgdspeak_la_CFLAGS = $(ESPEAK_CFLAGS) $(GLIB_CFLAGS) $(GSTREAMER_CFLAGS) $(defines)
libgdspeak_la_LIBADD = $(ESPEAK_LIBS) $(GLIB_LIBS) $(GSTREAMER_LIBS)
libgdspeak_la_LDFLAGS = -lm -no-undefined -version-info 0:0:0

libgdspeak_includedir=$(includedir)/gdspeak/
libgdspeak_include_HEADERS = gdspeak.h

libgdbusspeak_la_SOURCES = gdbus-speak.c gdbus-speak.h
libgdbusspeak_la_CFLAGS = $(GLIB_CFLAGS) $(defines)
libgdbusspeak_la_LIBADD = $(GLIB_LIBS) libgdspeak.la
libgdbusspeak_la_LDFLAGS = -lm -no-undefined -version-info 0:0:0

libgdbusspeak_includedir=$(includedir)/gdspeak/
libgdbusspeak_include_HEADERS = gdspeak-client-glue.h gdbus-speak.h

libexec_PROGRAMS=gdspeak
bin_PROGRAMS=gdspeak-client

gdspeak_CFLAGS = $(GLIB_CFLAGS) $(ESPEAK_CFLAGS) $(GSTREAMER_CFLAGS)
gdspeak_LDADD = $(GLIB_LIBS) $(ESPEAK_LIBS) $(GSTREAMER_LIBS) libgdbusspeak.la
gdspeak_SOURCES=main.c gdspeak-server-glue.h

gdspeak_client_CFLAGS = $(GLIB_CFLAGS)
gdspeak_client_LDADD = $(GLIB_LIBS)
gdspeak_client_SOURCES=client.c gdspeak-client-glue.h

BUILT_SOURCES=gdspeak-server-glue.h gdspeak-client-glue.h gdspeak-marshal.c gdspeak-marshal.h

EXTRA_DIST=gdspeak-marshal.list gdspeak-interface.xml org.tal.gdspeak.service.in

CLEANFILES=org.tal.gdspeak.service gdspeak-server-glue.h gdspeak-client-glue.h

gdspeak-server-glue.h: $(srcdir)/gdspeak-interface.xml
	$(DBUS_BINDING_TOOL) --mode=glib-server --prefix=gdspeak $(srcdir)/gdspeak-interface.xml > gdspeak-server-glue.h

gdspeak-client-glue.h: $(srcdir)/gdspeak-interface.xml
	$(DBUS_BINDING_TOOL) --mode=glib-client --prefix=gdspeak $(srcdir)/gdspeak-interface.xml > gdspeak-client-glue.h

servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.tal.gdspeak.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

gdspeak-marshal.c: gdspeak-marshal.list
	echo "#include \"gdspeak-marshal.h\"" > $@
	glib-genmarshal --prefix _gdspeak --body $< >> $@

gdspeak-marshal.h: gdspeak-marshal.list
	glib-genmarshal --internal --prefix _gdspeak --header $< > $@

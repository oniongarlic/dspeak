INCLUDES = -DDATADIR=\""$(datadir)"\" -DDBUS_API_SUBJECT_TO_CHANGE

libexec_PROGRAMS=dspeak

dspeak_CFLAGS = $(GLIB_CFLAGS) $(ESPEAK_CFLAGS)
dspeak_LDADD = $(GLIB_LIBS) $(ESPEAK_LIBS)

dspeak_SOURCES=main.c speak.c speak.h
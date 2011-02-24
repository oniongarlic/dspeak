AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(dspeak, 0.0.1)
AM_MAINTAINER_MODE

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CPP
AC_CONFIG_HEADERS(config.h)
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_DEFINE(_GNU_SOURCE, 1, [Enable GNU Extensions])
AC_CONFIG_MACRO_DIR(m4)
# GTK_DOC_CHECK(1.9)
PKG_PROG_PKG_CONFIG

dnl Debugging options to enable debug macros
DEBUG_FLAGS=
AC_ARG_ENABLE(debug,     [  --enable-debug                compile with DEBUG],,enable_debug=no)

if test "x$enable_debug" = "xyes"; then
    DEBUG_FLAGS="$DEBUG_FLAGS -DDEBUG -g -fno-omit-frame-pointer"
else
    DEBUG_FLAGS="$DEBUG_FLAGS -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT"
fi

CFLAGS="$CFLAGS $DEBUG_FLAGS -Wall -Wextra -Wmissing-prototypes -Wmissing-declarations"

PKG_CHECK_MODULES(GLIB, glib-2.0 dbus-1 dbus-glib-1 gconf-2.0)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)

DBUS_SERVICES_DIR="${datadir}/dbus-1/services"
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for D-Bus is])

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

# espeak
AC_CHECK_HEADER(espeak/speak_lib.h, HAVE_ESPEAK=yes, HAVE_ESPEAK=no)
if test "x$HAVE_ESPEAK" = "xyes"; then
	ESPEAK_LIBS="-lespeak"
	ESPEAK_CFLAGS="-I/usr/include/espeak"
	AC_CHECK_LIB([espeak], [espeak_Initialize],
		AC_DEFINE(WITH_ESPEAK, 1, [Defined if you have espeak])
		AC_SUBST(ESPEAK_CFLAGS)
		AC_SUBST(ESPEAK_LIBS))
else
  AC_MSG_ERROR([espeak is required.])
fi

# Produce output files
AC_OUTPUT(
	Makefile \
	dspeak.pc \
	dspeak/Makefile
)


EXTRA_DIST = autogen.sh

SUBDIRS = gdspeak

# DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
DISTCHECK_CONFIGURE_FLAGS=

.PHONY: ChangeLog
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log > $@; \
	fi

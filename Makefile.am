EXTRA_DIST = autogen.sh

SUBDIRS = gdspeak

# DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
DISTCHECK_CONFIGURE_FLAGS=

.PHONY: ChangeLog
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log > $@; \
	fi

DISTCLEANFILES = gdspeak-0.0.pc
EXTRA_DIST = gdspeak-0.0.pc.in ChangeLog

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdspeak-0.0.pc

upload-to-ftp: dist-gz dist-bz2
	cp $(distdir).tar.bz2 /talinux/pub/src/gdspeak/0.0/
	cp $(distdir).tar.gz /talinux/pub/src/gdspeak/0.0/
	md5sum $(distdir).tar.bz2 > /talinux/pub/src/gdspeak/0.0/$(distdir).tar.bz2.md5
	md5sum $(distdir).tar.gz > /talinux/pub/src/gdspeak/0.0/$(distdir).tar.gz.md5

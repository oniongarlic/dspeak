SUBDIRS = gdspeak docs

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

.PHONY: ChangeLog
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log > $@; \
	fi

DISTCLEANFILES = gdspeak-0.0.pc
EXTRA_DIST = gdspeak-0.0.pc.in ChangeLog autogen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdspeak-0.0.pc

# Private developer rules below
#
upload-all: upload-to-ftp upload-docs-to-ftp

upload-to-ftp: dist-gzip
	cp $(distdir).tar.gz /talinux/pub/src/gdspeak/0.0/
	md5sum $(distdir).tar.gz > /talinux/pub/src/gdspeak/0.0/$(distdir).tar.gz.md5

#	cp $(distdir).tar.bz2 /talinux/pub/src/gdspeak/0.0/
#	md5sum $(distdir).tar.bz2 > /talinux/pub/src/gdspeak/0.0/$(distdir).tar.bz2.md5

upload-docs-to-ftp:
	install -d -m 755 /talinux/pub/src/gdspeak/0.0/doc
	install -m 644 docs/reference/gdspeak/html/* /talinux/pub/src/gdspeak/0.0/doc/

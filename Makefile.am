## Process this file with automake to produce Makefile.in

SUBDIRS = @PROG_SUBDIRS@

install-data-local:
	@$(NORMAL_INSTALL)
	    if test -d $(srcdir)/share; then \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir); \
		for file in $(srcdir)/share/*; do \
		    if test -f $$file; then \
			$(INSTALL_DATA) $$file $(DESTDIR)$(pkgdatadir); \
		    fi \
		done \
	    fi

dist-hook:
	if test -d $(srcdir)/share; then \
	    mkdir $(distdir)/share; \
	    (cd $(srcdir)/share ; tar --exclude=.svn --exclude=gtkrc -cf - .) | (cd $(distdir)/share ; tar -xf -); \
	fi

docs:
	doxygen $(srcdir)/doxygen.cfg

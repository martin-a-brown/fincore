AUTOMAKE_OPTIONS = dist-bzip2
EXTRA_DIST = \
             README.packaging \
             $(PACKAGE).spec.in

bin_PROGRAMS = fincore

dist-hook:
	sed -e \
          's|@VERSION\@|$(VERSION)|g' \
          < $(srcdir)/$(PACKAGE).spec.in \
          > $(distdir)/$(PACKAGE).spec

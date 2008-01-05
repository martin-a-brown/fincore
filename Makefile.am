AUTOMAKE_OPTIONS = dist-bzip2
EXTRA_DIST = README README.* fincore.spec.in

bin_PROGRAMS = fincore

dist-hook:
	sed -e 's|@VERSION\@|$(VERSION)|g' < $(PACKAGE).spec.in > $(distdir)/$(PACKAGE).spec

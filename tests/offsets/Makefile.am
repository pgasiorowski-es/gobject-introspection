include $(top_srcdir)/common.mk
include $(top_srcdir)/Makefile.introspection

BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
INTROSPECTION_GIRS =

EXTRA_LTLIBRARIES =
EXTRA_PROGRAMS =

############################################################

EXTRA_LTLIBRARIES += liboffsets.la

liboffsets_la_SOURCES = \
	offsets.h	\
	offsets.c
liboffsets_la_CPPFLAGS = $(GIREPO_CFLAGS) $(GI_HIDDEN_VISIBILITY_CFLAGS) -I$(top_srcdir)/tests
liboffsets_la_CFLAGS = $(WARN_CFLAGS)
# dummy rpath to get built dynamically (huh?)
liboffsets_la_LDFLAGS = $(WARN_LDFLAGS) -avoid-version -rpath $(libdir)

Offsets-1.0.gir: liboffsets.la offsets.h
Offsets_1_0_gir_INCLUDES = GObject-2.0
Offsets_1_0_gir_LIBS = liboffsets.la
Offsets_1_0_gir_PACKAGES = gobject-2.0
Offsets_1_0_gir_FILES = offsets.h offsets.c
Offsets_1_0_gir_SCANNERFLAGS = $(INTROSPECTION_SCANNER_ARGS) --warn-all --warn-error  -I$(top_srcdir)/tests
INTROSPECTION_GIRS += Offsets-1.0.gir

CLEANFILES += Offsets-1.0.gir Offsets-1.0.typelib liboffsets.la

############################################################

EXTRA_PROGRAMS += gitestoffsets

nodist_gitestoffsets_SOURCES = gitestoffsets.c
gitestoffsets_CPPFLAGS = $(GIREPO_CFLAGS) -I$(top_srcdir)/girepository -I$(top_srcdir)/tests
gitestoffsets_CFLAGS = $(WARN_CFLAGS)
gitestoffsets_LDFLAGS = $(WARN_LDFLAGS)
gitestoffsets_LDADD = $(top_builddir)/libgirepository-1.0.la $(GIREPO_LIBS)

gitestoffsets.c: gen-gitestoffsets offsets.h Offsets-1.0.typelib
	$(AM_V_GEN) $(PYTHON) $(srcdir)/gen-gitestoffsets $(srcdir)/offsets.h > $@ || ( rm -f $@ && false )

EXTRA_DIST += \
	gen-gitestoffsets \
	test_offsets.py
BUILT_SOURCES += gitestoffsets.c
CLEANFILES += gitestoffsets.c $(EXTRA_PROGRAMS)

############################################################

# The trailing "echo" in LOG_COMPILER is used to ignore automake passing
# gitestoffsets as we already run this in the same command line.
TESTS = gitestoffsets
TESTS_ENVIRONMENT = env top_builddir=$(top_builddir) builddir=$(builddir)
LOG_COMPILER = $(PYTHON) $(srcdir)/test_offsets.py && echo


SUBDIRS = xp

if WIKIPEDIA_BUILTIN
noinst_LTLIBRARIES = wikipedia.la

else

plugindir = $(ABIWORD_PLUGINSDIR)
plugin_LTLIBRARIES = wikipedia.la

endif

wikipedia_la_LIBADD = \
	xp/libxp.la

wikipedia_la_LDFLAGS = \
	$(WIKIPEDIA_LIBS) \
	-avoid-version \
	-module \
	-no-undefined

wikipedia_la_SOURCES = 
nodist_EXTRA_wikipedia_la_SOURCES = dummy.cpp

EXTRA_DIST = \
	plugin.m4


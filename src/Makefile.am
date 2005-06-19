

pythondir = ${PYTHON_LIBP}/site-packages


python_LTLIBRARIES = UNDERSCORE_meanwhile.la

python_SCRIPTS = meanwhile.py

noinst_HEADERS = py_meanwhile.h


UNDERSCORE_meanwhile_la_SOURCES = \
	py_meanwhile.c \
	py_service.c \
	py_session.c \
	py_srvc_aware.c \
	py_srvc_conf.c \
	py_srvc_im.c \
	py_srvc_resolve.c \
	py_srvc_store.c

UNDERSCORE_meanwhile_la_CFLAGS = \
	$(DEBUG_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(MEANWHILE_CFLAGS) $(PYTHON_CFLAGS)

UNDERSCORE_meanwhile_la_LDFLAGS = \
	-module -avoid-version \
	@os_ld_flags_python@

UNDERSCORE_meanwhile_la_LIBADD = \
	$(GLIB_LIBS) \
	$(MEANWHILE_LIBS) $(PYTHON_LIBS)

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"@PACKAGE@\"


noinst_DATA = .libs/renamed
.libs/renamed:
	( cd .libs ; \
	  for F in UNDERSCORE_* ; do \
		cp $$F `echo $$F | sed s/UNDERSCORE//` ; \
          done ; \
	  touch renamed )


install-data-hook:
	( cd $(pythondir) ; \
	  rm -f UNDERSCORE_meanwhile.*a ; \
	  for F in UNDERSCORE_meanwhile* ; do \
		mv $$F `echo $$F | sed s/UNDERSCORE//` ; \
          done )


clean-pyc:
	( rm -f src/*.pyc )


all-local:	clean-pyc


clean-local:	clean-pyc



# version of pymeanwhile
m4_define(pymeanwhile_version,		1.0.0cvs)

# required minimum version of meanwhile
m4_define(meanwhile_min_version,	0.5.0)
m4_define(meanwhile_max_version,	0.6.0)

# required min version of glib2.
m4_define(glib_required_version,	2.0.0)

# this is just to generate the requirement in the specfile. It would
# be useful to make it check the available python version here as
# well...
m4_define(python_required_version,	2.2)



AC_INIT
AM_INIT_AUTOMAKE(meanwhile-python, pymeanwhile_version)

RELEASE=1
AC_SUBST(RELEASE)

AC_PREREQ([2.50])

AM_MAINTAINER_MODE

AC_PROG_CC

AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"

AC_PROG_INSTALL

AC_HEADER_STDC
AC_HEADER_SYS_WAIT

AC_C_CONST
AC_STRUCT_TM



# tell pkgconfig to look in the same prefix we're installing this to,
# as that's likely where meanwhile will be found if it's not in the
# default pkgconfig path

PREFIX=$ac_default_prefix
if test "$prefix" != "NONE"; then
   PREFIX=$prefix
fi

PKG_CONFIG_PATH="$PKG_CONFIG_PATH":"$PREFIX/lib/pkgconfig"
export PKG_CONFIG_PATH



# Debugging option
AC_ARG_ENABLE(debug,
	[  --enable-debug	  compile with debugging support],
	AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.]), )



# Darwin...
if test `uname` = "Darwin" ; then
	os_so_ext=".dylib"
	os_ld_flags="-dynamic-lib"
	os_ld_flags_python=os_ld_flags
else	
	os_so_ext=".so"
	os_ld_flags="-shared -no-undefined"
	os_ld_flags_python="-Wl,-rpath,${libdir}"
fi
AC_SUBST(os_so_ext)
AC_SUBST(os_ld_flags)
AC_SUBST(os_ld_flags_python)



# Python wrapper option
withval="auto"
AC_ARG_WITH(python,
	[  --with-python[=auto]   compile Python wrappers for Meanwhile])

# Check 1: Python binary
if test "$withval" = "auto" ; then
	AC_PATH_PROG(pythonpath, python)
	if test "_$pythonpath" = _ ; then
		AC_MSG_ERROR(no python found)
	fi
else
	pythonpath="$withval"
fi

PYTHON="$pythonpath"
AC_SUBST(PYTHON)


# Check 2: Python version
AC_MSG_CHECKING(Python version)
PYTHON_FOUND_VERSION=`$pythonpath -c 'import distutils.sysconfig; print "%s" % distutils.sysconfig.get_config_var("VERSION")' 2>/dev/null`

if test "$PYTHON_FOUND_VERSION" = "" ; then
	AC_MSG_ERROR([unable to determine version])
else

	# TODO: put in a check against PYTHON_FOUND_VERSION and
	# python_required_version to make sure we meet the minimum
	# requirements

	AC_MSG_RESULT($PYTHON_FOUND_VERSION)
	#PYTHON_VERSION=python_required_version
	PYTHON_VERSION=$PYTHON_FOUND_VERSION
	AC_SUBST(PYTHON_VERSION)
fi


# Check 3: Python CFLAGS
AC_MSG_CHECKING(for Python compile flags)
PYTHON_CFLAGS=`$pythonpath -c 'import distutils.sysconfig; print "-I%s" % distutils.sysconfig.get_python_inc()' 2>/dev/null`

if test "$PYTHON_CFLAGS" = "" ; then
	AC_MSG_ERROR(not found)
else
	AC_MSG_RESULT($PYTHON_CFLAGS)
	AC_SUBST(PYTHON_CFLAGS)
fi


# Check 4: Python LIBS
AC_MSG_CHECKING(for Python linker flags)
PYTHON_LIBP=`$pythonpath -c 'import distutils.sysconfig; print "%s" % distutils.sysconfig.get_config_var("LIBP")' 2>/dev/null`

PYTHON_LIBS=`$pythonpath -c 'import distutils.sysconfig; print "-L%s %s" % (distutils.sysconfig.get_config_var("LIBP"), distutils.sysconfig.get_config_var("LIBS"))' 2>/dev/null`

if test "$PYTHON_LIBS" = "" ; then
	AC_MSG_ERROR(not found)
else

	if test `uname` = "Darwin" ; then
		PYTHON_LIBS="-framework Python $PYTHON_LIBS"
	fi

	AC_MSG_RESULT($PYTHON_LIBS)
	AC_SUBST(PYTHON_LIBS)
	AC_SUBST(PYTHON_LIBP)
fi

WITH_PYTHON_SUBPACKAGE=1
AC_SUBST(WITH_PYTHON_SUBPACKAGE)

WITH_PYTHON_SUBDIR="python"
AC_SUBST(WITH_PYTHON_SUBDIR)



# Glib-2.0
AM_PATH_GLIB_2_0(glib_required_version, ,
	AC_MSG_ERROR([*** GLib glib_required_version is required.])
 )
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)



# Meanwhile required, of course
PKG_CHECK_MODULES(MEANWHILE,
	[meanwhile >= meanwhile_min_version]
	[meanwhile < meanwhile_max_version],
	[AC_DEFINE(HAVE_MEANWHILE, 1, [We've found meanwhile])]
)

AC_SUBST(MEANWHILE_CFLAGS)
AC_SUBST(MEANWHILE_LIBS)

AC_SUBST(MEANWHILE_MIN_VERSION)
AC_SUBST(MEANWHILE_MAX_VERSION)

AC_CONFIG_FILES(
	[Makefile src/Makefile]
	[meanwhile-python.spec]
)

AC_CONFIG_FILES([test/test.sh], [chmod +x test/test.sh])

AC_OUTPUT()
